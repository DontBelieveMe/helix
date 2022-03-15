#define COUNT (100)

static int primes[COUNT + 1];

int main() {
	int i,j;

	for (i = 2; i <= COUNT; ++i)
		primes[i] = i;

	i = 2;

	while ((i * i) <= COUNT) {
		if (primes[i] != 0) {
			for (j = 2; j < COUNT; ++j) {
				if (primes[i] * j > COUNT)
					break;

				primes[primes[i] * j] = 0;
			}
		}

		i++;
	}

	const int expected[] =
	{
	    2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41,
	    43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97
	};

	int nth = 0;

	for (i = 2; i < COUNT; ++i) {
		if (primes[i] != 0) {
			if (primes[i] != expected[nth])
				return 1;

			nth++;
		}
	}

	return 0;
}
