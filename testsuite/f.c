typedef int my_integer;
typedef my_integer mi;

struct hello_world
{
	int v;
};

typedef struct hello_world shw;

mi add(mi a, mi b) {
	return a + b;
}

void addg(mi a, int b, struct hello_world c, shw  d) { }

int main()
{
}
