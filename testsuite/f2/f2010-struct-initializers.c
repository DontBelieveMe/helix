struct ST {
	int a;
	char* b;
	short c;
};

struct ST gs = { 50, "nine", 655 };

void single_struct()
{
	struct ST s = { 100, "tester", 123 };
}

void struct_array()
{
	struct ST s[] =
	{
		{ 10, "hello world!", 20 },
		{ 20, "hello",        10 },
		{ 30, "world",        0  }
	};
}