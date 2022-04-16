/* Basic case, hoists 32 bit integer */
int f1() {
  return 120;
}

/* Testing that constants of different types are hoisted correctly */
char f2() {
  return 4;
}

/* Testing a 32 bit number gets split into two 16 bit moves */
int f3() {
  return 123712312;
}
