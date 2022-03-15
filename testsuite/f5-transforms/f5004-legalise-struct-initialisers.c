struct E { int c; };

struct ST { int a; int b; int c; struct E d; };

int main() {
  struct ST a = { 10, 20, 30, { 40 } };
  return a.d.c;
}