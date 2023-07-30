#include <stdio.h>
#include <stdlib.h>


double fit_sphere(const gsl_vector *x, void *params)
{
    double total = 0.0;
    for (int i = 0; i < x->size; i++) {
        double d = gsl_vector_get(x, i);
        total += d * d;
    }
    gsl_vector_fprintf(stdout, x, "%f");
    printf(" In sphere %f\n", total);
    return total;
}

int main(int argc, char *argv[])
{


}
