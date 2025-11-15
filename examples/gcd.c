int
gcd (int a, int b)
{
  while (b != 0)
    {
      int resto = a % b;
      a = b;
      b = resto;
    }
  return a;
}

int
main ()
{
  int x;
  int y;
  int a = 2;
  x = 2 / 2;
  y = 1;
  y = 10;
  if (y == 1)
    {
      y = 10;
    }
  return gcd (x, y);
  int z = 10;
}
