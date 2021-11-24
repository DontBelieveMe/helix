int main()
{
	for (int i = 0; i < 10; i = i + 1) {
		for (int j = 5; j >= 0; j = j - 1) {
			if (j <= 2) {
				continue;
			}
		}

		if (i>=7) {
			continue;
		}
	}

	return 0;
}