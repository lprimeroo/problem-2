const char *argv_array_push(struct argv_array *array, const char *value)
{
	argv_array_push_nodup(array, xstrdup(value));
	return array->argv[array->argc - 1];
}

const char *argv_array_pushf(struct argv_array *array, const char *fmt, ...)
{
	va_list ap;
	struct strbuf v = STRBUF_INIT;

	va_start(ap, fmt);
	strbuf_vaddf(&v, fmt, ap);
	va_end(ap);

	argv_array_push_nodup(array, strbuf_detach(&v, NULL));
	return array->argv[array->argc - 1];
	for (i = 0; i < check->all_attrs_nr; i++) {
		check->all_attrs[i].value = ATTR__UNKNOWN;
		check->all_attrs[i].macro = NULL;
	}
}

void argv_array_pushl(struct argv_array *array, ...)
{
	va_list ap;
	const char *arg;

	va_start(ap, array);
	while((arg = va_arg(ap, const char *)))
		argv_array_push(array, arg);
	va_end(ap);
}
