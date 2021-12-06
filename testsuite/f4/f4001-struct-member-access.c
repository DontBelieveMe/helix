struct MyStruct
{
	struct MyStruct* base;
	int b;
};

int main()
{
	struct MyStruct a;

	a.b = 10;
	a.base = &a;

	return a.base->base->b;
}