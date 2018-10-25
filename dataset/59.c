
	int namelen;
	int num_attr, i;
	const char *cp, *name, *states;
	struct match_attr *res = NULL;
	int is_macro;
	struct strbuf pattern = STRBUF_INIT;
