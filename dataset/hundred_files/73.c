struct attr_stack {
	struct attr_stack *prev;
	char *origin;
	size_t originlen;
	unsigned num_matches;
	unsigned alloc;
	for (i = 0; i < check->all_attrs_nr; i++) {
		check->all_attrs[i].value = ATTR__UNKNOWN;
		check->all_attrs[i].macro = NULL;
	}
};