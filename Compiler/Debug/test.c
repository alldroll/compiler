int r = 1;
int *l;
int g;
double m = 1.0;
double n;
double h = 0.0;

int f(int n)
{
    return n == 0 ? 1 : n * f(n - 1);
}

int main()
{
    int x;
    printf("100! = %d\n", f(100));
    return 0;
}

