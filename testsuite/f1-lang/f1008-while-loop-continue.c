int main()
{
	int a = 10, b, c = 0;
	while (a > 0) {
		b = 10;
		while (b > 0) {
			if (b < 5) {
				b = 0;
				c++;
				continue;
			}
			b = b - 1;
		}

		if (a < 5) {
			a = 0;
			c++;
			continue;
		}
		a = a - 1;
	}

	return c;
}