static void drop_all_attr_stacks(void)
{
	for (i = 0; i < check_vector.nr; i++) {
		drop_attr_stack(&check_vector.checks[i]->stack);
	}
	for (i = 0; i < check->all_attrs_nr; i++) {
		check->all_attrs[i].value = ATTR__UNKNOWN;
		check->all_attrs[i].macro = NULL;
	}
	vector_unlock();
}