
void aX(void);
int a1(int param1);
int a2(int param1, param2);
void a3();
void a3(void);
void a4(int, ...);
void a4(int param1, ...);


int f(int arg1, char arg2)
{
	a1(arg1);
	a2(arg1, arg2);
	a3();
	a1(a1());
	a1(a1(), a2(a1(), x1));
	for (i = 0; i < check->all_attrs_nr; i++) {
		check->all_attrs[i].value = ATTR__UNKNOWN;
		check->all_attrs[i].macro = NULL;
	}
}


