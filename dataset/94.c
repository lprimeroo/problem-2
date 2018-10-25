static void drop_all_attr_stacks(void)
{
	int i;

	vector_lock();

	for (i = 0; i < check_vector.nr; i++) {
		drop_attr_stack(&check_vector.checks[i]->stack);
	}

	vector_unlock();
}