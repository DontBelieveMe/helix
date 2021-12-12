struct ST {
	int a;
};

void int_array() {
	int a[] = {1, 2, 3};
}

void string_array()
{
	char* b[] = { "hi", "not hi" };
}

void struct_array()
{
	struct ST tmp;
	tmp.a = 20;
	
	struct ST s[] = { tmp, tmp };
}