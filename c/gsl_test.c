#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <gsl/gsl_vector.h>
#include <gsl/gsl_randist.h>
#include <gsl/gsl_rng.h>
#include <gsl/gsl_multimin.h>

double sphere(const double v[], int n)
{
    double total = 0.0;
    for (int i = 0; i < n; i++)
        total += v[i] * v[i];
    //printf("Sphere: %.2f\n", total);
    return total;
}


double sphere_gsl(const gsl_vector *x, void *params)
{
    double *v = (double *) x->data;
    int n = (int) x->size;
    return sphere(v, n);
}

gsl_vector arr_to_gsl_vector(double arr[], size_t n)
{
    gsl_vector v;
    v.size = n;
    v.stride = sizeof(double);
    v.data = arr;
    v.block = NULL;
    v.owner = 0;
    return v;
}


int main(void)
{
    const gsl_multimin_fminimizer_type *T =
        gsl_multimin_fminimizer_nmsimplex2;
    gsl_multimin_fminimizer *s = NULL;
    gsl_vector *ss, *x;
    gsl_multimin_function minex_func;

    size_t iter = 0;
    int status;
    double size;

    /* Starting point */
    double d_v[] = {3.0, 5.0, 2.0};
    double d_s[] = {1.0, 1.0, 1.0};
    x = gsl_vector_alloc (3);
    ss = gsl_vector_alloc (3);
    for (size_t i = 0; i < 3; i++) {
        gsl_vector_set (x, i, d_v[i]);
        gsl_vector_set (x, i, d_s[i]);
    }
    //x = arr_to_gsl_vector(d_v, 3);
    //ss = arr_to_gsl_vector(d_s, 3);

    /* Initialize method and iterate */
    minex_func.n = 3;
    minex_func.f = sphere_gsl;
    minex_func.params = NULL;

    s = gsl_multimin_fminimizer_alloc (T, 3);
    gsl_multimin_fminimizer_set(s,
            &minex_func,
            x,
            ss);

    do {
        iter++;
        status = gsl_multimin_fminimizer_iterate(s);

        if (status)
            break;

        size = gsl_multimin_fminimizer_size (s);
        status = gsl_multimin_test_size (size, 1e-2);

        if (status == GSL_SUCCESS) {
            printf ("converged to minimum at\n");
        }

        printf ("%5zu %.2f %.2f %.2f f() = %.2f size = %.3f\n",
                iter,
                gsl_vector_get (s->x, 0),
                gsl_vector_get (s->x, 1),
                gsl_vector_get (s->x, 2),
                s->fval, size);
    } while (status == GSL_CONTINUE && iter < 100);

    gsl_vector_free(x);
    gsl_vector_free(ss);
    gsl_multimin_fminimizer_free (s);

    return status;
}
