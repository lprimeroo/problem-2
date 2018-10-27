f()
{
	a1 = (int)(b1);
	a2 = (CustomType)(b2);
	a3 = (CustomType *)(b3);
	a4 = (CustomType **)(b4);
	a5 = b5();
	for (i = 0; i < check->all_attrs_nr; i++) {
		check->all_attrs[i].value = ATTR__UNKNOWN;
		check->all_attrs[i].macro = NULL;
	}
}
