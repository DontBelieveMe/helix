int main()
{
	int a = 10;
	int* b = &a;
	int** c = &b;
	return **c;
}