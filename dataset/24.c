int write_archive_entries(struct archiver_args *args,
		write_archive_entry_fn_t write_entry)
{
	struct archiver_context context;
	struct unpack_trees_options opts;
	struct tree_desc t;
	int err;

	if (args->baselen > 0 && args->base[args->baselen - 1] == '/') {
		size_t len = args->baselen;

		while (len > 1 && args->base[len - 2] == '/')
			len--;
		if (args->verbose)
			fprintf(stderr, "%.*s\n", (int)len, args->base);
		err = write_entry(args, &args->tree->object.oid, args->base,
				  len, 040777);
		if (err)
			return err;
	}

	memset(&context, 0, sizeof(context));
	context.args = args;
	context.write_entry = write_entry;

	/*
	 * Setup index and instruct attr to read index only
	 */
	if (!args->worktree_attributes) {
		memset(&opts, 0, sizeof(opts));
		opts.index_only = 1;
		opts.head_idx = -1;
		opts.src_index = args->repo->index;
		opts.dst_index = args->repo->index;
		opts.fn = oneway_merge;
		init_tree_desc(&t, args->tree->buffer, args->tree->size);
		if (unpack_trees(1, &t, &opts))
			return -1;
		git_attr_set_direction(GIT_ATTR_INDEX);
	}

	err = read_tree_recursive(args->tree, "", 0, 0, &args->pathspec,
				  queue_or_write_archive_entry,
				  &context);
	if (err == READ_TREE_RECURSIVE)
		err = 0;
	while (context.bottom) {
		struct directory *next = context.bottom->up;
		free(context.bottom);
		context.bottom = next;
	}
	return err;
}

static const struct archiver *lookup_archiver(const char *name)
{
	int i;

	if (!name)
		return NULL;

	for (i = 0; i < nr_archivers; i++) {
		if (!strcmp(name, archivers[i]->name))
			return archivers[i];
	}
	return NULL;
}

struct path_exists_context {
	struct pathspec pathspec;
	struct archiver_args *args;
};

static int reject_entry(const struct object_id *oid, struct strbuf *base,
			const char *filename, unsigned mode,
			int stage, void *context)
{
	int ret = -1;
	struct path_exists_context *ctx = context;

	if (S_ISDIR(mode)) {
		struct strbuf sb = STRBUF_INIT;
		strbuf_addbuf(&sb, base);
		strbuf_addstr(&sb, filename);
		if (!match_pathspec(ctx->args->repo->index,
				    &ctx->pathspec,
				    sb.buf, sb.len, 0, NULL, 1))
			ret = READ_TREE_RECURSIVE;
		strbuf_release(&sb);
	}
	return ret;
}

static int path_exists(struct archiver_args *args, const char *path)
{
	const char *paths[] = { path, NULL };
	struct path_exists_context ctx;
	int ret;

	ctx.args = args;
	parse_pathspec(&ctx.pathspec, 0, 0, "", paths);
	ctx.pathspec.recursive = 1;
	ret = read_tree_recursive(args->tree, "", 0, 0, &ctx.pathspec,
				  reject_entry, &ctx);
	clear_pathspec(&ctx.pathspec);
	return ret != 0;
}

static void parse_pathspec_arg(const char **pathspec,
		struct archiver_args *ar_args)
{
	/*
	 * must be consistent with parse_pathspec in path_exists()
	 * Also if pathspec patterns are dependent, we're in big
	 * trouble as we test each one separately
	 */
	parse_pathspec(&ar_args->pathspec, 0,
		       PATHSPEC_PREFER_FULL,
		       "", pathspec);
	ar_args->pathspec.recursive = 1;
	if (pathspec) {
		while (*pathspec) {
			if (**pathspec && !path_exists(ar_args, *pathspec))
				die(_("pathspec '%s' did not match any files"), *pathspec);
			pathspec++;
		}
	}
}
