#include <stdlib.h>

int main();
void __libc_main(void);

void
__libc_main(void)
{
	const int exit_code = main();
	exit(exit_code);
}
