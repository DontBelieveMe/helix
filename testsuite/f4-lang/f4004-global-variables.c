int a;
int b = 20;

int main()
{
	a = 30;

	int* pb = &b;
	*pb = 30;

	return a + b;
}