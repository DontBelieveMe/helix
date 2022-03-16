#include <string.h>

void*
memset(void* ptr, int value, size_t num)
{
	char* bytes = (char*) ptr;

	for (size_t i = 0; i < num; ++i)
		bytes[i] = (char) value;

	return ptr;
}
