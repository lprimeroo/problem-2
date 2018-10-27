/*
Func call with various arg numbers.
And func forward declarations.
*/

void aX(void);
int a1(int param1);
int a2(int param1, param2);
void a3();
void a3(void);

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


