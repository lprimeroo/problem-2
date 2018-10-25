static int write_tar_entry(struct archiver_args *args,
			   const struct object_id *oid,
			   const char *path, size_t pathlen,
			   unsigned int mode)
{
	struct ustar_header header;
	struct strbuf ext_header = STRBUF_INIT;
	unsigned int old_mode = mode;
	unsigned long size, size_in_header;
	void *buffer;
	int err = 0;

	memset(&header, 0, sizeof(header));

	if (S_ISDIR(mode) || S_ISGITLINK(mode)) {
		*header.typeflag = TYPEFLAG_DIR;
		mode = (mode | 0777) & ~tar_umask;
	} else if (S_ISLNK(mode)) {
		*header.typeflag = TYPEFLAG_LNK;
		mode |= 0777;
	} else if (S_ISREG(mode)) {
		*header.typeflag = TYPEFLAG_REG;
		mode = (mode | ((mode & 0100) ? 0777 : 0666)) & ~tar_umask;
	} else {
		return error(_("unsupported file mode: 0%o (SHA1: %s)"),
			     mode, oid_to_hex(oid));
	}
	if (pathlen > sizeof(header.name)) {
		size_t plen = get_path_prefix(path, pathlen,
					      sizeof(header.prefix));
		size_t rest = pathlen - plen - 1;
		if (plen > 0 && rest <= sizeof(header.name)) {
			memcpy(header.prefix, path, plen);
			memcpy(header.name, path + plen + 1, rest);
		} else {
			xsnprintf(header.name, sizeof(header.name), "%s.data",
				  oid_to_hex(oid));
			strbuf_append_ext_header(&ext_header, "path",
						 path, pathlen);
		}
	} else
		memcpy(header.name, path, pathlen);

	if (S_ISREG(mode) && !args->convert &&
	    oid_object_info(args->repo, oid, &size) == OBJ_BLOB &&
	    size > big_file_threshold)
		buffer = NULL;
	else if (S_ISLNK(mode) || S_ISREG(mode)) {
		enum object_type type;
		buffer = object_file_to_archive(args, path, oid, old_mode, &type, &size);
		if (!buffer)
			return error(_("cannot read %s"), oid_to_hex(oid));
	} else {
		buffer = NULL;
		size = 0;
	}

	if (S_ISLNK(mode)) {
		if (size > sizeof(header.linkname)) {
			xsnprintf(header.linkname, sizeof(header.linkname),
				  "see %s.paxheader", oid_to_hex(oid));
			strbuf_append_ext_header(&ext_header, "linkpath",
			                         buffer, size);
		} else
			memcpy(header.linkname, buffer, size);
	}

	size_in_header = size;
	if (S_ISREG(mode) && size > USTAR_MAX_SIZE) {
		size_in_header = 0;
		strbuf_append_ext_header_uint(&ext_header, "size", size);
	}

	prepare_header(args, &header, mode, size_in_header);

	if (ext_header.len > 0) {
		write_extended_header(args, oid, ext_header.buf,
				      ext_header.len);
	}
	strbuf_release(&ext_header);
	write_blocked(&header, sizeof(header));
	if (S_ISREG(mode) && size > 0) {
		if (buffer)
			write_blocked(buffer, size);
		else
			err = stream_blocked(oid);
	}
	free(buffer);
	return err;
}

static void write_global_extended_header(struct archiver_args *args)
{
	const unsigned char *sha1 = args->commit_sha1;
	struct strbuf ext_header = STRBUF_INIT;
	struct ustar_header header;
	unsigned int mode;

	if (sha1)
		strbuf_append_ext_header(&ext_header, "comment",
					 sha1_to_hex(sha1), 40);
	if (args->time > USTAR_MAX_MTIME) {
		strbuf_append_ext_header_uint(&ext_header, "mtime",
					      args->time);
		args->time = USTAR_MAX_MTIME;
	}

	if (!ext_header.len)
		return;

	memset(&header, 0, sizeof(header));
	*header.typeflag = TYPEFLAG_GLOBAL_HEADER;
	mode = 0100666;
	xsnprintf(header.name, sizeof(header.name), "pax_global_header");
	prepare_header(args, &header, mode, ext_header.len);
	write_blocked(&header, sizeof(header));
	write_blocked(ext_header.buf, ext_header.len);
	strbuf_release(&ext_header);
}

static struct archiver **tar_filters;
static int nr_tar_filters;
static int alloc_tar_filters;

static struct archiver *find_tar_filter(const char *name, int len)
{
	int i;
	for (i = 0; i < nr_tar_filters; i++) {
		struct archiver *ar = tar_filters[i];
		if (!strncmp(ar->name, name, len) && !ar->name[len])
			return ar;
	}
	return NULL;
}

static int tar_filter_config(const char *var, const char *value, void *data)
{
	struct archiver *ar;
	const char *name;
	const char *type;
	int namelen;

	if (parse_config_key(var, "tar", &name, &namelen, &type) < 0 || !name)
		return 0;

	ar = find_tar_filter(name, namelen);
	if (!ar) {
		ar = xcalloc(1, sizeof(*ar));
		ar->name = xmemdupz(name, namelen);
		ar->write_archive = write_tar_filter_archive;
		ar->flags = ARCHIVER_WANT_COMPRESSION_LEVELS;
		ALLOC_GROW(tar_filters, nr_tar_filters + 1, alloc_tar_filters);
		tar_filters[nr_tar_filters++] = ar;
	}

	if (!strcmp(type, "command")) {
		if (!value)
			return config_error_nonbool(var);
		free(ar->data);
		ar->data = xstrdup(value);
		return 0;
	}
	if (!strcmp(type, "remote")) {
		if (git_config_bool(var, value))
			ar->flags |= ARCHIVER_REMOTE;
		else
			ar->flags &= ~ARCHIVER_REMOTE;
		return 0;
	}

	return 0;
}

static int git_tar_config(const char *var, const char *value, void *cb)
{
	if (!strcmp(var, "tar.umask")) {
		if (value && !strcmp(value, "user")) {
			tar_umask = umask(0);
			umask(tar_umask);
		} else {
			tar_umask = git_config_int(var, value);
		}
		return 0;
	}

	return tar_filter_config(var, value, cb);
}

static int write_tar_archive(const struct archiver *ar,
			     struct archiver_args *args)
{
	int err = 0;

	write_global_extended_header(args);
	err = write_archive_entries(args, write_tar_entry);
	if (!err)
		write_trailer();
	return err;
}

static int write_tar_filter_archive(const struct archiver *ar,
				    struct archiver_args *args)
{
	struct strbuf cmd = STRBUF_INIT;
	struct child_process filter = CHILD_PROCESS_INIT;
	const char *argv[2];
	int r;

	if (!ar->data)
		BUG("tar-filter archiver called with no filter defined");

	strbuf_addstr(&cmd, ar->data);
	if (args->compression_level >= 0)
		strbuf_addf(&cmd, " -%d", args->compression_level);

	argv[0] = cmd.buf;
	argv[1] = NULL;
	filter.argv = argv;
	filter.use_shell = 1;
	filter.in = -1;

	if (start_command(&filter) < 0)
		die_errno(_("unable to start '%s' filter"), argv[0]);
	close(1);
	if (dup2(filter.in, 1) < 0)
		die_errno(_("unable to redirect descriptor"));
	close(filter.in);

	r = write_tar_archive(ar, args);

	close(1);
	if (finish_command(&filter) != 0)
		die(_("'%s' filter reported error"), argv[0]);

	strbuf_release(&cmd);
	return r;
}

static struct archiver tar_archiver = {
	"tar",
	write_tar_archive,
	ARCHIVER_REMOTE
};

void init_tar_archiver(void)
{
	int i;
	register_archiver(&tar_archiver);

	tar_filter_config("tar.tgz.command", "gzip -cn", NULL);
	tar_filter_config("tar.tgz.remote", "true", NULL);
	tar_filter_config("tar.tar.gz.command", "gzip -cn", NULL);
	tar_filter_config("tar.tar.gz.remote", "true", NULL);
	git_config(git_tar_config, NULL);
	for (i = 0; i < nr_tar_filters; i++) {
		/* omit any filters that never had a command configured */
		if (tar_filters[i]->data)
			register_archiver(tar_filters[i]);
	}
}
