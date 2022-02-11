int main()
{
	int a = 10;
	int* b = &a;
	int** c = &b;

	*b = 3;
	**c = 4;

	return **c;
}