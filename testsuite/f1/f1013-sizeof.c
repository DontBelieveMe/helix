unsigned long long size_int() {
	return sizeof(int);
}

unsigned long long size_uchar() {
	return sizeof(unsigned char);
}

unsigned long long size_schar() {
	return sizeof(signed char);
}

unsigned long long size_short() {
	return sizeof(short);
}

unsigned long long size_long_long() {
	return sizeof(long long);
}

unsigned long long size_char_ptr() {
	return sizeof(char*);
}

unsigned long long array_type() {
	return sizeof(int[10]);
}