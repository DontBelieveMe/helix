int
main()
{
	int x;
	int y;

	x = 0;
	y = x++;

	if (x != 1)
		return 13;

	if (y != 0)
		return 31;

	return 0;
}
