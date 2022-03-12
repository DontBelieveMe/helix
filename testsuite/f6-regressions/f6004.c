/* Test calling functions that return a simple value
 * and don't take any parameters */

int dosimple(void)
{
	/* Could just trivially return a constant here
	 * but do some simple work, and get some things on the
	 * stack */
	int c = 10, a = 0;

	while (c > 0) {
		a += 2;
		c--;
	}

	return a;
}

int main()
{
	int a = dosimple();

	for (int i = 0; i < 3; ++i)
		a += dosimple();

	/* Deliberately discard result, to check
	 * that the call is not overwriting anything obvious. */
	dosimple();

	const int c = dosimple() / 4;

	return a + dosimple() + c;
}
