static const struct git_attr *git_attr_internal(const char *name, int namelen)
{
	struct git_attr *a;

	if (!attr_name_valid(name, namelen))
		return NULL;

	hashmap_lock(&g_attr_hashmap);

	a = attr_hashmap_get(&g_attr_hashmap, name, namelen);

	if (!a) {
		FLEX_ALLOC_MEM(a, name, name, namelen);
		a->attr_nr = hashmap_get_size(&g_attr_hashmap.map);

		attr_hashmap_add(&g_attr_hashmap, a->name, namelen, a);
		assert(a->attr_nr ==
		       (hashmap_get_size(&g_attr_hashmap.map) - 1));
	}

	hashmap_unlock(&g_attr_hashmap);

	return a;
}

const struct git_attr *git_attr(const char *name)
{
	return git_attr_internal(name, strlen(name));
}

/* What does a matched pattern decide? */
struct attr_state {
	const struct git_attr *attr;
	const char *setto;
};

struct pattern {
	const char *pattern;
	int patternlen;
	int nowildcardlen;
	unsigned flags;		/* EXC_FLAG_* */
};

/*
 * One rule, as from a .gitattributes file.
 *
 * If is_macro is true, then u.attr is a pointer to the git_attr being
 * defined.
 *
 * If is_macro is false, then u.pat is the filename pattern to which the
 * rule applies.
 *
 * In either case, num_attr is the number of attributes affected by
 * this rule, and state is an array listing them.  The attributes are
 * listed as they appear in the file (macros unexpanded).
 */
struct match_attr {
	union {
		struct pattern pat;
		const struct git_attr *attr;
	} u;
	char is_macro;
	unsigned num_attr;
	struct attr_state state[FLEX_ARRAY];
};
