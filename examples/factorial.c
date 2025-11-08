int factorial(int n)
{
    if (n == 0 || n == 1)
    {
        return 1;
    }
    else
    {
        return n * factorial(n - 1);
    }
    if (n) {
        n = n;
    }
}

int main()
{
    int n = 1;
    int m = 2;
    n = 10;
    return factorial(n);
}