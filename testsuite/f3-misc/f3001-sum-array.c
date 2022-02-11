int sum(int* array, int array_length)
{
	int sum = 0;
	for (int i = 0; i < array_length; i++) {
		sum += array[i];
	}
	return sum;
}