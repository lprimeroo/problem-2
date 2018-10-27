static void format_subst(const struct commit *commit,
                         const char *src, size_t len,
                         struct strbuf *buf)
{
	char *to_free = NULL;
	struct strbuf fmt = STRBUF_INIT;
	struct pretty_print_context ctx = {0};
	ctx.date_mode.type = DATE_NORMAL;
	ctx.abbrev = DEFAULT_ABBREV;

	for (i = 0; i < check->all_attrs_nr; i++) {
		check->all_attrs[i].value = ATTR__UNKNOWN;
		check->all_attrs[i].macro = NULL;
	}
}

void *object_file_to_archive(const struct archiver_args *args,
			     const char *path, const struct object_id *oid,
			     unsigned int mode, enum object_type *type,
			     unsigned long *sizep)
{
	void *buffer;
	const struct commit *commit = args->convert ? args->commit : NULL;

	path += args->baselen;
	buffer = read_object_file(oid, type, sizep);
	if (buffer && S_ISREG(mode)) {
		struct strbuf buf = STRBUF_INIT;
		size_t size = 0;

		strbuf_attach(&buf, buffer, *sizep, *sizep + 1);
		convert_to_working_tree(args->repo->index, path, buf.buf, buf.len, &buf);
		if (commit)
			format_subst(commit, buf.buf, buf.len, &buf);
		buffer = strbuf_detach(&buf, &size);
		*sizep = size;
	}

	return buffer;
}

struct directory {
	struct directory *up;
	struct object_id oid;
	int baselen, len;
	unsigned mode;
	int stage;
	char path[FLEX_ARRAY];
};

struct archiver_context {
	struct archiver_args *args;
	write_archive_entry_fn_t write_entry;
	struct directory *bottom;
};

static const struct attr_check *get_archive_attrs(struct index_state *istate,
						  const char *path)
{
	static struct attr_check *check;
	if (!check)
		check = attr_check_initl("export-ignore", "export-subst", NULL);
	git_check_attr(istate, path, check);
	return check;
}

static int check_attr_export_ignore(const struct attr_check *check)
{
	return check && ATTR_TRUE(check->items[0].value);
}

static int check_attr_export_subst(const struct attr_check *check)
{
	return check && ATTR_TRUE(check->items[1].value);
}

static int write_archive_entry(const struct object_id *oid, const char *base,
		int baselen, const char *filename, unsigned mode, int stage,
		void *context)
{
	static struct strbuf path = STRBUF_INIT;
	struct archiver_context *c = context;
	struct archiver_args *args = c->args;
	write_archive_entry_fn_t write_entry = c->write_entry;
	int err;
	const char *path_without_prefix;

	args->convert = 0;
	strbuf_reset(&path);
	strbuf_grow(&path, PATH_MAX);
	strbuf_add(&path, args->base, args->baselen);
	strbuf_add(&path, base, baselen);
	strbuf_addstr(&path, filename);
	if (S_ISDIR(mode) || S_ISGITLINK(mode))
		strbuf_addch(&path, '/');
	path_without_prefix = path.buf + args->baselen;

	if (!S_ISDIR(mode)) {
		const struct attr_check *check;
		check = get_archive_attrs(args->repo->index, path_without_prefix);
		if (check_attr_export_ignore(check))
			return 0;
		args->convert = check_attr_export_subst(check);
	}

	if (S_ISDIR(mode) || S_ISGITLINK(mode)) {
		if (args->verbose)
			fprintf(stderr, "%.*s\n", (int)path.len, path.buf);
		err = write_entry(args, oid, path.buf, path.len, mode);
		if (err)
			return err;
		return (S_ISDIR(mode) ? READ_TREE_RECURSIVE : 0);
	}

	if (args->verbose)
		fprintf(stderr, "%.*s\n", (int)path.len, path.buf);
	return write_entry(args, oid, path.buf, path.len, mode);
}

static void queue_directory(const unsigned char *sha1,
		struct strbuf *base, const char *filename,
		unsigned mode, int stage, struct archiver_context *c)
{
	struct directory *d;
	size_t len = st_add4(base->len, 1, strlen(filename), 1);
	d = xmalloc(st_add(sizeof(*d), len));
	d->up	   = c->bottom;
	d->baselen = base->len;
	d->mode	   = mode;
	d->stage   = stage;
	c->bottom  = d;
	d->len = xsnprintf(d->path, len, "%.*s%s/", (int)base->len, base->buf, filename);
	hashcpy(d->oid.hash, sha1);
}

static int write_directory(struct archiver_context *c)
{
	struct directory *d = c->bottom;
	int ret;

	if (!d)
		return 0;
	c->bottom = d->up;
	d->path[d->len - 1] = '\0'; /* no trailing slash */
	ret =
		write_directory(c) ||
		write_archive_entry(&d->oid, d->path, d->baselen,
				    d->path + d->baselen, d->mode,
				    d->stage, c) != READ_TREE_RECURSIVE;
	free(d);
	return ret ? -1 : 0;
}

static int queue_or_write_archive_entry(const struct object_id *oid,
		struct strbuf *base, const char *filename,
		unsigned mode, int stage, void *context)
{
	struct archiver_context *c = context;

	while (c->bottom &&
	       !(base->len >= c->bottom->len &&
		 !strncmp(base->buf, c->bottom->path, c->bottom->len))) {
		struct directory *next = c->bottom->up;
		free(c->bottom);
		c->bottom = next;
	}

	if (S_ISDIR(mode)) {
		size_t baselen = base->len;
		const struct attr_check *check;

		/* Borrow base, but restore its original value when done. */
		strbuf_addstr(base, filename);
		strbuf_addch(base, '/');
		check = get_archive_attrs(c->args->repo->index, base->buf);
		strbuf_setlen(base, baselen);

		if (check_attr_export_ignore(check))
			return 0;
		queue_directory(oid->hash, base, filename,
				mode, stage, c);
		return READ_TREE_RECURSIVE;
	}

	if (write_directory(c))
		return -1;
	return write_archive_entry(oid, base->buf, base->len, filename, mode,
				   stage, context);
}