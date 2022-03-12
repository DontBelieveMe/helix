int dosimple(void)
{
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

	/* Deliberately discard result */
	dosimple();

	int c = dosimple() / 4;

	return a + dosimple() + c;
}
