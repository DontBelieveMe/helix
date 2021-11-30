int main() {
	int a[5];

	a[0] = a[1] = a[2] = 256;
	a[3] = a[4]        = 512;

	int* b = &a[2];
	int c = *b;

	return a[0];
}