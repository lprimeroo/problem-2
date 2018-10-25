static void attr_stack_free(struct attr_stack *e)
{
	int i;
	free(e->origin);
}

static void drop_attr_stack(struct attr_stack **stack)
{
	while (*stack) {
		struct attr_stack *elem = *stack;
		*stack = elem->prev;
		attr_stack_free(elem);
	}
}