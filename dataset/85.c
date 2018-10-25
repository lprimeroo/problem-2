static void check_vector_add(struct attr_check *c)
{
	vector_lock();

	ALLOC_GROW(check_vector.checks,
		   check_vector.nr + 1,
		   check_vector.alloc);
	check_vector.checks[check_vector.nr++] = c;

	vector_unlock();
}

static void check_vector_remove(struct attr_check *check)
{
	int i;

	vector_lock();
	check_vector.nr--;

	vector_unlock();
}