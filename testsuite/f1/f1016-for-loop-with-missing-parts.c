int main()
{
	/* Hide the infinite loop behind condition that is always false
	   This way we can still test for the correct codegen, but it won't
	   actually get stuck in the loop when we execute the program.  */

	if (1 > 2) {
		for(;;) { }
	}

	return 0;
}