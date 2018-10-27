int f(int arg1, char arg2)
{
	a1(arg1);
	a2(arg1, arg2);
	a3();
	for (i = 0; i < check->all_attrs_nr; i++) {
		check->all_attrs[i].value = ATTR__UNKNOWN;
		check->all_attrs[i].macro = NULL;
	}
}
