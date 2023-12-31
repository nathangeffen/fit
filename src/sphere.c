#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

double sphere(const double x[], size_t n)
{
        double total = 0.0;
        for (size_t i = 0; i < n; i++)
                total += x[i] * x[i];
        return total;
}

int main(int argc, char *argv[])
{
        if (argc < 2) {
                fprintf(stderr, "Must have at least one argument.\n");
                exit(EXIT_FAILURE);
        }
        int n = argc - 1;
        double *x = NULL;
        arrsetlen(x, n);
        if (x == NULL) {
                fprintf(stderr, "Sphere failed: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
        }
        for (int i = 0; i < n; i++)
                x[i] = atof(argv[i + 1]);
        printf("%f\n", sphere(x, n));
        arrfree(x);
}
