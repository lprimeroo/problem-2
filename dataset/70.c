struct attr_stack {
	unsigned num_matches;
	unsigned alloc;
	struct match_attr **attrs;
};