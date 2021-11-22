/*int f0() {
	int a = 10;
	a = 30;
	return a;
}

int f1() {
	int a;
	a = 50;
	return a;
}
*/
int f2() {
	int a;
	int b = (a = 1);
	return b;
}