struct MyType {
  int a;
  int b;
};

int via_array_acces(struct MyType* a) {
  return a[1].a;
}

int via_direct_ptr_access(struct MyType* a) {
  return a->b;
}

int via_value_access() {
  struct MyType ty;
  ty.a = 20;
  return ty.a;
}

