

struct all_attrs_item {
	const struct git_attr *attr;
	const char *value;
	/*
	 * If 'macro' is non-NULL, indicates that 'attr' is a macro based on
	 * the current attribute stack and contains a pointer to the match_attr
	 * definition of the macro
	 */
	const struct match_attr *macro;
};