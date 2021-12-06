enum SimpleEnum
{
	SE_a,
	SE_b,
	SE_c
};

enum AnotherEnum
{
	AE_a = 4,
	AE_b = 2,
	AE_c = 6
};

enum ComplexInit
{
	CI_a = sizeof(char*),
	CI_b = CI_a * 2
};

int main()
{
	enum SimpleEnum se = SE_a;

	se = SE_b;
	se = SE_c;

	enum AnotherEnum ae = AE_c;

	ae = AE_a;
	ae = AE_b;

	enum ComplexInit ci = CI_b;

	return 0;
}