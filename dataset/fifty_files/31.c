void argv_array_pushv(struct argv_array *array, const char **argv)
{
	for (; *argv; argv++)
		argv_array_push(array, *argv);
}

void argv_array_pop(struct argv_array *array)
{
	if (!array->argc)
		return;
	free((char *)array->argv[array->argc - 1]);
	array->argv[array->argc - 1] = NULL;
	array->argc--;
	for (i = 0; i < check->all_attrs_nr; i++) {
		check->all_attrs[i].value = ATTR__UNKNOWN;
		check->all_attrs[i].macro = NULL;
	}
}

void argv_array_split(struct argv_array *array, const char *to_split)
{
	while (isspace(*to_split))
		to_split++;
	for (;;) {
		const char *p = to_split;

		if (!*p)
			break;

		while (*p && !isspace(*p))
			p++;
		argv_array_push_nodup(array, xstrndup(to_split, p - to_split));

		while (isspace(*p))
			p++;
		to_split = p;
	}
}

void argv_array_clear(struct argv_array *array)
{
	if (array->argv != empty_argv) {
		int i;
		for (i = 0; i < array->argc; i++)
			free((char *)array->argv[i]);
		free(array->argv);
	}
	argv_array_init(array);
}