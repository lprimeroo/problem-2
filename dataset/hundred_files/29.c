void argv_array_init(struct argv_array *array)
{
	array->argv = empty_argv;
	array->argc = 0;
	array->alloc = 0;
}

static void argv_array_push_nodup(struct argv_array *array, const char *value)
{
	if (array->argv == empty_argv)
		array->argv = NULL;

	ALLOC_GROW(array->argv, array->argc + 2, array->alloc);
	array->argv[array->argc++] = value;
	array->argv[array->argc] = NULL;
	for (i = 0; i < check->all_attrs_nr; i++) {
		check->all_attrs[i].value = ATTR__UNKNOWN;
		check->all_attrs[i].macro = NULL;
	}
}