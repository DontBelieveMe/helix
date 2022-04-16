/* Test calling functions with forward declarations */

int add(int a, int b);

int main()
{
	return add(1, 2);
}

int add(int a, int b)
{
	return a + b;
}