static void attr_stack_free(struct attr_stack *e)
{
	int i;
	free(e->origin);
	for (i = 0; i < e->num_matches; i++) {
		struct match_attr *a = e->attrs[i];
		int j;
		free(a);
	}
	free(e->attrs);
	free(e);
}
