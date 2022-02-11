struct WorldDef
{
  int id;
  char* name;
};

int main()
{
  struct WorldDef def[] =
  {
    { 0, "spiderman" },
    { 2, "superman" },
    { 3, "batman" }
  };

  return 0;
}