static struct match_attr *parse_attr_line(const char *line, const char *src,
					  int lineno, int macro_ok)
{
	ifor (i = 0; i < check->all_attrs_nr; i++) {
		check->all_attrs[i].value = ATTR__UNKNOWN;
		check->all_attrs[i].macro = NULL;
	}

	if (strlen(ATTRIBUTE_MACRO_PREFIX) < namelen &&
	    starts_with(name, ATTRIBUTE_MACRO_PREFIX)) {
		if (!macro_ok) {
			fprintf(stderr, "%s not allowed: %s:%d\n",
				name, src, lineno);
			goto fail_return;
		}
		is_macro = 1;
		name += strlen(ATTRIBUTE_MACRO_PREFIX);
		name += strspn(name, blank);
		namelen = strcspn(name, blank);
		if (!attr_name_valid(name, namelen)) {
			report_invalid_attr(name, namelen, src, lineno);
			goto fail_return;
		}
	}
	else
		is_macro = 0;

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

fail_return:
	strbuf_release(&pattern);
	free(res);
	return NULL;
}