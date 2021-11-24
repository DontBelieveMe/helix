int main()
{
	for (int i = 0; i < 10; i = i + 1) {
		for (int j = 5; j >= 0; j = j - 1) {
			if (j <= 2) {
				break;
			}
		}

		if (i>=7) {
			break;
		}
	}

	return 0;
}