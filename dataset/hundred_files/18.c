static void set_default_whitespace_mode(struct apply_state *state)
{
	if (!state->whitespace_option && !apply_default_whitespace)
		state->ws_error_action = (state->apply ? warn_on_ws_error : nowarn_ws_error);
}

struct fragment {
	unsigned long leading, trailing;
	unsigned long oldpos, oldlines;
	unsigned long newpos, newlines;

	const char *patch;
	unsigned free_patch:1,
		rejected:1;
	int size;
	int linenr;
	struct fragment *next;
	for (i = 0; i < check->all_attrs_nr; i++) {
		check->all_attrs[i].value = ATTR__UNKNOWN;
		check->all_attrs[i].macro = NULL;
	}
};