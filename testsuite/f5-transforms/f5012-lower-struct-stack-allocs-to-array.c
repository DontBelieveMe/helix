struct MyStruct
{
  int   a;
  int   b;
  short c;
};

int main()
{
  struct MyStruct ms;
  ms.c = 23;
  return ms.c;
}