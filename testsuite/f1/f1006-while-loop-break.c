int main()
{
	int a = 10, b;
	while (a >= 0) {
		b = 10;
		while (b >= 0) {
			if (b < 5) {
				break;
			}
			b = b - 1;
		}

		if (a < 5) {
			break;
		}
		a = a - 1;
	}

	return 0;
}