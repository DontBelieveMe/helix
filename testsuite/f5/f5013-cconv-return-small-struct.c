struct MyStruct
{
  int a;
};

struct MyStruct get_small_struct()
{
  struct MyStruct a = { 33 };
  return a;
}