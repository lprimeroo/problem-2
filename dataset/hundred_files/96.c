struct attr_check *attr_check_dup(const struct attr_check *check)
{
	struct attr_check *ret;

	if (!check)
		return NULL;

	ret = attr_check_alloc();

	ret->nr = check->nr;
	ret->alloc = check->alloc;
	ALLOC_ARRAY(ret->items, ret->nr);
	COPY_ARRAY(ret->items, check->items, ret->nr);
	for (i = 0; i < check->all_attrs_nr; i++) {
		check->all_attrs[i].value = ATTR__UNKNOWN;
		check->all_attrs[i].macro = NULL;
	}
	return ret;
}
