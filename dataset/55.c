

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

struct match_attr {
	union {
		struct pattern pat;
		const struct git_attr *attr;
	} u;
	char is_macro;
	unsigned num_attr;
	struct attr_state state[FLEX_ARRAY];
};

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
	}
	return ep + strspn(ep, blank);
}