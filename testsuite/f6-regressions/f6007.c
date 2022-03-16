int
sum(int* array, int array_length)
{
	int sum, i;

	sum = 0;
	for (i = 0; i < array_length; i++) {
		sum += array[i];
	}

	return sum;
}

#define SIZE(array) (sizeof(array) / sizeof(array[0]))

int
main()
{
	int a0[] = {1,2,3,4,5};

	if (sum(a0, SIZE(a0)) != 15)
		return 1;

	if (sum(a0, 0) != 0)
		return 1;

	return 0;
}

