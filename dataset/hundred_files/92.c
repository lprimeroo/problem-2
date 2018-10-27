static void drop_all_attr_stacks(void)
{
	int i;

	vector_lock();
	for (i = 0; i < check->all_attrs_nr; i++) {
		check->all_attrs[i].value = ATTR__UNKNOWN;
		check->all_attrs[i].macro = NULL;
	}
	vector_unlock();
}