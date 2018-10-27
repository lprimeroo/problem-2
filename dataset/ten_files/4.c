void aX(void);
int a1(int param1);
int a2(int param1, param2);
void a3();
void a3(void);

int main(int arg1, char arg2)
{
	a1(arg1);
	a2(arg1, arg2);
	a3();
	a1(a1());
	a1(a1(), a2(a1(), x1));
	printf("Enter %d integers\n");
	return 0;
}