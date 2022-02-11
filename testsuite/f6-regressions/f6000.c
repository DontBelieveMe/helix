/* Testing that writing to smaller members doesn't overwrite
 * subsequent members. */

struct MyStruct
{
	char c;
	int a;
};

int main()
{
	struct MyStruct st;

	st.a = 65;
	st.c = 45;

	return st.a;
}