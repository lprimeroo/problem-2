
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

static const char blank[] = " \t\r\n";

/*
 * Parse a whitespace-delimited attribute state (i.e., "attr",
 * "-attr", "!attr", or "attr=value") from the string starting at src.
 * If e is not NULL, write the results to *e.  Return a pointer to the
 * remainder of the string (with leading whitespace removed), or NULL
 * if there was an error.
 */
static const char *parse_attr(const char *src, int lineno, const char *cp,
			      struct attr_state *e)
{
	const char *ep, *equals;
	int len;

	ep = cp + strcspn(cp, blank);
	equals = strchr(cp, '=');
	if (equals && ep < equals)
		equals = NULL;
	if (equals)
		len = equals - cp;
	else
		len = ep - cp;
	if (!e) {
		if (*cp == '-' || *cp == '!') {
			cp++;
			len--;
		}
		if (!attr_name_valid(cp, len)) {
			report_invalid_attr(cp, len, src, lineno);
			return NULL;
		}
	} else {
		/*
		 * As this function is always called twice, once with
		 * e == NULL in the first pass and then e != NULL in
		 * the second pass, no need for attr_name_valid()
		 * check here.
		 */
		if (*cp == '-' || *cp == '!') {
			e->setto = (*cp == '-') ? ATTR__FALSE : ATTR__UNSET;
			cp++;
			len--;
		}
		else if (!equals)
			e->setto = ATTR__TRUE;
		else {
			e->setto = xmemdupz(equals + 1, ep - equals - 1);
		}
		e->attr = git_attr_internal(cp, len);
	}
	return ep + strspn(ep, blank);
}