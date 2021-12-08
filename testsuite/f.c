enum a
{
  F_HI = 1,
  F_NO = F_HI * 2
};

struct MyStruct
{
  int a;
  int b;
};

enum a DoThingToStruct(struct MyStruct* s)
{
  if (s->a == s->b)
    return F_HI;
  else
    return F_NO;
}

int main()
{
  struct MyStruct tmp;
  tmp.a = tmp.b = 120;
  return DoThingToStruct(&tmp);
}