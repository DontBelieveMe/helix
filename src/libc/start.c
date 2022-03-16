#include "sys.h"

int main();
void __libc_main(void);

void
__libc_main(void)
{
	const int exit_code = main();
	__syscall1(_SYS_EXIT, exit_code);
}
