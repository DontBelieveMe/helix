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

/* struct_array test */
int main()
{
	struct ST tmp;
	tmp.a = 20;
	
	struct ST s[] = { tmp, tmp };

	return s[0].a;
}