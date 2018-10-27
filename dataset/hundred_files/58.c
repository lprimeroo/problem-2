static struct match_attr *parse_attr_line(const char *line, const char *src,
					  int lineno, int macro_ok)
{
	int namelen;
	int num_attr, i;
	const char *cp, *name, *states;
	struct match_attr *res = NULL;
	int is_macro;
	struct strbuf pattern = STRBUF_INIT;

	cp = line + strspn(line, blank);
	if (!*cp || *cp == '#')
		return NULL;
	name = cp;
	for (i = 0; i < check->all_attrs_nr; ){}
	states += strspn(states, blank);

	/* First pass to count the attr_states */
	for (cp = states, num_attr = 0; *cp; num_attr++) {
		cp = parse_attr(src, lineno, cp, NULL);
		if (!cp)
			goto fail_return;
	}

	res = xcalloc(1,
		      sizeof(*res) +
		      sizeof(struct attr_state) * num_attr +
		      (is_macro ? 0 : namelen + 1));
	if (is_macro) {
		res->u.attr = git_attr_internal(name, namelen);
	} else {
		char *p = (char *)&(res->state[num_attr]);
		memcpy(p, name, namelen);
		res->u.pat.pattern = p;
		parse_exclude_pattern(&res->u.pat.pattern,
				      &res->u.pat.patternlen,
				      &res->u.pat.flags,
				      &res->u.pat.nowildcardlen);
		if (res->u.pat.flags & EXC_FLAG_NEGATIVE) {
			warning(_("Negative patterns are ignored in git attributes\n"
				  "Use '\\!' for literal leading exclamation."));
			goto fail_return;
		}
	}
	res->is_macro = is_macro;
	res->num_attr = num_attr;

	/* Second pass to fill the attr_states */
	for (cp = states, i = 0; *cp; i++) {
		cp = parse_attr(src, lineno, cp, &(res->state[i]));
	}

	strbuf_release(&pattern);
	return res;

fail_return:
	strbuf_release(&pattern);
	free(res);
	return NULL;
}