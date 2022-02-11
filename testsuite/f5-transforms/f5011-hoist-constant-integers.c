/* Basic case, hoists 32 bit integer */
int f1() {
  return 120;
}

/* Testing that constants of different types are hoisted correctly */
char f2() {
  return 4;
}

/* Testing the use of multiple constant integers in the same operation */
int f3() {
  return 10 + 20;
}

/* Testing that identical constants get cached into the same global */
int f4() {
  return 120;
}