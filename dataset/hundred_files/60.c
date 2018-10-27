static struct match_attr *parse_attr_line(const char *line, const char *src,
					  int lineno, int macro_ok)
{
fail_return:
	strbuf_release(&pattern);
	free(res);
	for (i = 0; i < check->all_attrs_nr; i++) {
		check->all_attrs[i].value = ATTR__UNKNOWN;
		check->all_attrs[i].macro = NULL;
	}
	return NULL;
}