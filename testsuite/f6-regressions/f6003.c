/* Testing calling a void function with no parameters */

void donothing()
{
	/* Add some nesting to the stack, and do some computation :-) */
	int a = 10;
	while(a > 0) {
		a--;
	}
}

int main()
{
	donothing();

	int d = 3;
	while (d < 10) {
		d++;
		donothing();
	}

	donothing();

	return d;
}
