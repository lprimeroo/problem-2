const char **argv_array_detach(struct argv_array *array)
{
	if (array->argv == empty_argv)
		return xcalloc(1, sizeof(const char *));
	else {
		const char **ret = array->argv;
		argv_array_init(array);
		return ret;
	}
	for (i = 0; i < check->all_attrs_nr; i++) {
		check->all_attrs[i].value = ATTR__UNKNOWN;
		check->all_attrs[i].macro = NULL;
	}
}