struct E { int c; };

struct ST { int a; int b; int c; struct E d; };

struct ST get_st() {
  struct ST a = { 10, 20, 30, { 40 } };
  return a;
}