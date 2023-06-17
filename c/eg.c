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

/* Paraboloid centered on (p[0],p[1]), with
   scale factors (p[2],p[3]) and minimum p[4] */

double my_f(const gsl_vector * v, void *params)
{
	double total = 0.0;
	for (int i = 0; i < v->size; i++) {
		double d = gsl_vector_get(v, i);
		total += d * d;
	}
	return total;
}

int main(int argc, char *argv[])
{
	// double par[5] = {1.0, 2.0, 10.0, 20.0, 30.0};

	const gsl_multimin_fminimizer_type *T =
	    gsl_multimin_fminimizer_nmsimplex2;
	gsl_multimin_fminimizer *s = NULL;
	gsl_vector *ss, *x;
	gsl_multimin_function minex_func;

	size_t iter = 0;
	int status;
	double size;

	size_t n = (argc > 1) ? atoi(argv[1]) : 10;
    size_t max_iter = (argc > 2) ? atoi(argv[2]) : 1000;


	/* Starting point */
	x = gsl_vector_alloc(n);
	for (size_t i = 0; i < n; i++) {
		gsl_vector_set(x, i, i + 1);
	}

	/* Set initial step sizes to 1 */
	ss = gsl_vector_alloc(n);
	gsl_vector_set_all(ss, 1.0);

	/* Initialize method and iterate */
	minex_func.n = n;
	minex_func.f = my_f;
	minex_func.params = NULL;

	s = gsl_multimin_fminimizer_alloc(T, n);
	gsl_multimin_fminimizer_set(s, &minex_func, x, ss);

	do {
		iter++;
		status = gsl_multimin_fminimizer_iterate(s);

		if (status)
			break;

		size = gsl_multimin_fminimizer_size(s);
		status = gsl_multimin_test_size(size, 1e-2);

		if (status == GSL_SUCCESS) {
			printf("converged to minimum at\n");
		}

        if (status == GSL_SUCCESS || iter % 1000 == 0) {
            printf("Iter: %zu: ", iter);
            for (size_t i = 0; i < n; i++) {
                printf("%.2f ", gsl_vector_get(s->x, i));
            }
            printf("f() %.2f size: %.2f\n", s->fval, size);
        }
	}
	while (status == GSL_CONTINUE && iter < max_iter);

	gsl_vector_free(x);
	gsl_vector_free(ss);
	gsl_multimin_fminimizer_free(s);

	return status;
}
