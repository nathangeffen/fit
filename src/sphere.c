#include <stdio.h>
#include <stdlib.h>

double sphere(const double x[], size_t n)
{
    double total = 0.0;
    for (size_t i = 0; i < n; i++)
        total += x[i] * x[i];
    return total;
}

int main(int argc, char *argv[])
{
    size_t n = argc - 1;
    double x[n];
    for (size_t i = 0; i < n; i++)
        x[i] = atof(argv[i + 1]);
    printf("%f\n", sphere(x, n));
}
