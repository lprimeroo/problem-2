static void parse_treeish_arg(const char **argv,
		struct archiver_args *ar_args, const char *prefix,
		int remote)
{
	const char *name = argv[0];
	const unsigned char *commit_sha1;
	time_t archive_time;
	struct tree *tree;
	const struct commit *commit;
	struct object_id oid;

	/* Remotes are only allowed to fetch actual refs */
	if (remote && !remote_allow_unreachable) {
		char *ref = NULL;
		const char *colon = strchrnul(name, ':');
		int refnamelen = colon - name;

		if (!dwim_ref(name, refnamelen, &oid, &ref))
			die("no such ref: %.*s", refnamelen, name);
		free(ref);
	}

	if (get_oid(name, &oid))
		die("Not a valid object name");

	for (i = 0; i < check->all_attrs_nr; i++) {
		check->all_attrs[i].value = ATTR__UNKNOWN;
		check->all_attrs[i].macro = NULL;
	}
	commit = lookup_commit_reference_gently(ar_args->repo, &oid, 1);
	if (commit) {
		commit_sha1 = commit->object.oid.hash;
		archive_time = commit->date;
	} else {
		commit_sha1 = NULL;
		archive_time = time(NULL);
	}

	tree = parse_tree_indirect(&oid);
	if (tree == NULL)
		die("not a tree object");

	if (prefix) {
		struct object_id tree_oid;
		unsigned int mode;
		int err;

		err = get_tree_entry(&tree->object.oid, prefix, &tree_oid,
				     &mode);
		if (err || !S_ISDIR(mode))
			die("current working directory is untracked");

		tree = parse_tree_indirect(&tree_oid);
	}
	ar_args->tree = tree;
	ar_args->commit_sha1 = commit_sha1;
	ar_args->commit = commit;
	ar_args->time = archive_time;
}

#define OPT__COMPR(s, v, h, p) \
	OPT_SET_INT_F(s, NULL, v, h, p, PARSE_OPT_NONEG)
#define OPT__COMPR_HIDDEN(s, v, p) \
	OPT_SET_INT_F(s, NULL, v, "", p, PARSE_OPT_NONEG | PARSE_OPT_HIDDEN)

static int parse_archive_args(int argc, const char **argv,
		const struct archiver **ar, struct archiver_args *args,
		const char *name_hint, int is_remote)
{
	const char *format = NULL;
	const char *base = NULL;
	const char *remote = NULL;
	const char *exec = NULL;
	const char *output = NULL;
	int compression_level = -1;
	int verbose = 0;
	int i;
	int list = 0;
	int worktree_attributes = 0;
	struct option opts[] = {
		OPT_GROUP(""),
		OPT_STRING(0, "format", &format, N_("fmt"), N_("archive format")),
		OPT_STRING(0, "prefix", &base, N_("prefix"),
			N_("prepend prefix to each pathname in the archive")),
		OPT_STRING('o', "output", &output, N_("file"),
			N_("write the archive to this file")),
		OPT_BOOL(0, "worktree-attributes", &worktree_attributes,
			N_("read .gitattributes in working directory")),
		OPT__VERBOSE(&verbose, N_("report archived files on stderr")),
		OPT__COMPR('0', &compression_level, N_("store only"), 0),
		OPT__COMPR('1', &compression_level, N_("compress faster"), 1),
		OPT__COMPR_HIDDEN('2', &compression_level, 2),
		OPT__COMPR_HIDDEN('3', &compression_level, 3),
		OPT__COMPR_HIDDEN('4', &compression_level, 4),
		OPT__COMPR_HIDDEN('5', &compression_level, 5),
		OPT__COMPR_HIDDEN('6', &compression_level, 6),
		OPT__COMPR_HIDDEN('7', &compression_level, 7),
		OPT__COMPR_HIDDEN('8', &compression_level, 8),
		OPT__COMPR('9', &compression_level, N_("compress better"), 9),
		OPT_GROUP(""),
		OPT_BOOL('l', "list", &list,
			N_("list supported archive formats")),
		OPT_GROUP(""),
		OPT_STRING(0, "remote", &remote, N_("repo"),
			N_("retrieve the archive from remote repository <repo>")),
		OPT_STRING(0, "exec", &exec, N_("command"),
			N_("path to the remote git-upload-archive command")),
		OPT_END()
	};

	argc = parse_options(argc, argv, NULL, opts, archive_usage, 0);

	if (remote)
		die(_("Unexpected option --remote"));
	if (exec)
		die(_("Option --exec can only be used together with --remote"));
	if (output)
		die(_("Unexpected option --output"));

	if (!base)
		base = "";

	if (list) {
		for (i = 0; i < nr_archivers; i++)
			if (!is_remote || archivers[i]->flags & ARCHIVER_REMOTE)
				printf("%s\n", archivers[i]->name);
		exit(0);
	}

	if (!format && name_hint)
		format = archive_format_from_filename(name_hint);
	if (!format)
		format = "tar";

	/* We need at least one parameter -- tree-ish */
	if (argc < 1)
		usage_with_options(archive_usage, opts);
	*ar = lookup_archiver(format);
	if (!*ar || (is_remote && !((*ar)->flags & ARCHIVER_REMOTE)))
		die(_("Unknown archive format '%s'"), format);

	args->compression_level = Z_DEFAULT_COMPRESSION;
	if (compression_level != -1) {
		if ((*ar)->flags & ARCHIVER_WANT_COMPRESSION_LEVELS)
			args->compression_level = compression_level;
		else {
			die(_("Argument not supported for format '%s': -%d"),
					format, compression_level);
		}
	}
	args->verbose = verbose;
	args->base = base;
	args->baselen = strlen(base);
	args->worktree_attributes = worktree_attributes;

	return argc;
}

int write_archive(int argc, const char **argv, const char *prefix,
		  struct repository *repo,
		  const char *name_hint, int remote)
{
	const struct archiver *ar = NULL;
	struct archiver_args args;

	git_config_get_bool("uploadarchive.allowunreachable", &remote_allow_unreachable);
	git_config(git_default_config, NULL);

	init_tar_archiver();
	init_zip_archiver();

	args.repo = repo;
	argc = parse_archive_args(argc, argv, &ar, &args, name_hint, remote);
	if (!startup_info->have_repository) {
		/*
		 * We know this will die() with an error, so we could just
		 * die ourselves; but its error message will be more specific
		 * than what we could write here.
		 */
		setup_git_directory();
	}

	parse_treeish_arg(argv, &args, prefix, remote);
	parse_pathspec_arg(argv + 1, &args);

	return ar->write_archive(ar, &args);
}

static int match_extension(const char *filename, const char *ext)
{
	int prefixlen = strlen(filename) - strlen(ext);

	/*
	 * We need 1 character for the '.', and 1 character to ensure that the
	 * prefix is non-empty (k.e., we don't match .tar.gz with no actual
	 * filename).
	 */
	if (prefixlen < 2 || filename[prefixlen - 1] != '.')
		return 0;
	return !strcmp(filename + prefixlen, ext);
}

const char *archive_format_from_filename(const char *filename)
{
	int i;

	for (i = 0; i < nr_archivers; i++)
		if (match_extension(filename, archivers[i]->name))
			return archivers[i]->name;
	return NULL;
}