/*
 * For test purposes only. Hence the use of variable length arrays.
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

void sphere_dx(const double x[], size_t n, double dx[])
{
        for (size_t i = 0; i < n; i++) {
                dx[i] = 2 * x[i];
        }
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

        double *dx = NULL;
        arrsetlen(dx, n);
        sphere_dx(x, n, dx);
        for (size_t i = 0; i < n; i++)
                printf("%f ", dx[i]);
        printf("\n");
        arrfree(x);
        arrfree(dx);
}
