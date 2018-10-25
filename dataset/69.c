struct attr_stack {
	struct attr_stack *prev;
	char *origin;
	size_t originlen;
};