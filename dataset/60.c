static struct match_attr *parse_attr_line(const char *line, const char *src,
					  int lineno, int macro_ok)
{
fail_return:
	strbuf_release(&pattern);
	free(res);
	return NULL;
}