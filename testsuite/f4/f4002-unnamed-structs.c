int main()
{
	struct { int a; int b; } s;

	s.a = 10;
	s.b = s.a * 2;

	return s.b;
}