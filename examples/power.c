int
power (int base, int exp)
{
  int result = 1;
  int b = 0; 

  while (exp > 0)
    {
      if (exp % 2 == 1)
        {
          result *= base;
        }
      base *= base;
      exp /= 2;
    }

  return result;
}

int
main ()
{
  int base;
  int exp; 
  base = 1; 
  base = 2;
  if (exp == 3) 
    {
      exp = 4;
    }
  exp = 1 * 5; 
  return power (base, exp);
  exp = 10; 
}
