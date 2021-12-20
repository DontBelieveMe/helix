typedef unsigned long size_t;

size_t size_int() {
	return sizeof(int);
}

size_t size_uchar() {
	return sizeof(unsigned char);
}

size_t size_schar() {
	return sizeof(signed char);
}

size_t size_short() {
	return sizeof(short);
}

size_t size_long_long() {
	return sizeof(long long);
}

size_t size_char_ptr() {
	return sizeof(char*);
}

size_t array_type() {
	return sizeof(int[10]);
}