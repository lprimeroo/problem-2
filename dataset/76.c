
static void drop_attr_stack(struct attr_stack **stack)
{
	while (*stack) {
		struct attr_stack *elem = *stack;
		*stack = elem->prev;
		attr_stack_free(elem);
	}
}