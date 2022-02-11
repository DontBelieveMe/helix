/* Testing reading members from global structs created
 * using a initialiser.  */

struct MyStruct
{
  int a;
  int b;
};

struct MyStruct ms = { 50, 20 };

int main()
{
  return ms.b;
}