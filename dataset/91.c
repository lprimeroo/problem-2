static void drop_all_attr_stacks(void)
{
	for (i = 0; i < check_vector.nr; i++) {
		drop_attr_stack(&check_vector.checks[i]->stack);
	}

	vector_unlock();
}