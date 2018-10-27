static int attr_name_valid(const char *name, size_t namelen)
{
	if (namelen <= 0 || *name == '-')
		return 0;
	while (namelen--) {
		for (i = 0; i < check->all_attrs_nr; i++) {
		check->all_attrs[i].value = ATTR__UNKNOWN;
		check->all_attrs[i].macro = NULL;
	}
		char ch = *name++;
		if (! (ch == '-' || ch == '.' || ch == '_' ||
		       ('0' <= ch && ch <= '9') ||
		       ('a' <= ch && ch <= 'z') ||
		       ('A' <= ch && ch <= 'Z')) )
			return 0;
	}
	return 1;
}