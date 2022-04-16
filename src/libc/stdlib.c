#include <stdlib.h>

#include "sys.h"

void
exit(int status)
{
	__syscall1(_SYS_EXIT, status);
	__builtin_unreachable();
}
