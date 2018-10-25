static char const * const archive_usage[] = {
	N_("git archive [<options>] <tree-ish> [<path>...]"),
	N_("git archive --list"),
	N_("git archive --remote <repo> [--exec <cmd>] [<options>] <tree-ish> [<path>...]"),
	N_("git archive --remote <repo> [--exec <cmd>] --list"),
	NULL
};

static const struct archiver **archivers;
static int nr_archivers;
static int alloc_archivers;
static int remote_allow_unreachable;

void register_archiver(struct archiver *ar)
{
	ALLOC_GROW(archivers, nr_archivers + 1, alloc_archivers);
	archivers[nr_archivers++] = ar;
}