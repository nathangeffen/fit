#ifndef FIT_H
#define FIT_H

#include <stdbool.h>
#include <stdint.h>
#include <gsl/gsl_errno.h>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_randist.h>
#include <gsl/gsl_rng.h>
#include <gsl/gsl_multimin.h>

#define FIT_STR_LEN 32
#define FIT_LONG_STR_LEN 200

typedef double (*fit_function)(size_t, const double[], void *);

struct fit_lo_hi {
        double lo;
        double hi;
};

struct fit_model {
        bool verbose;
        uint32_t variables;
        char function_name[FIT_STR_LEN];
        fit_function function;
        struct fit_lo_hi *lo_hi;
        bool free_lo_hi;
        uint32_t *divisions;
        bool free_divisions;
        uint32_t generations;
        uint32_t passes;
        uint32_t threads;
        uint32_t iterations;
        double min_error;
        char method[FIT_STR_LEN];
        char command[FIT_LONG_STR_LEN];
        double *best;
        double lowest;
        uint32_t func_calls;
        void *params;
        int error;
};

struct fit_model fit_model_default();
void fit_optimize(struct fit_model *model);

double fit_sphere(size_t n, const double x[], void *params);
double fit_rastrigin(size_t n, const double x[], void *params);
double fit_flipflop(size_t n, const double x[], void *params);
double fit_external(size_t n, const double x[], void *params);
char * vecf_str(size_t n, const double vec[]);
char *vecu_str(size_t n, const unsigned vec[]);
char *vec_lo_hi_str(size_t n, const struct fit_lo_hi vec[]);
void fit_model_print(const struct fit_model *model);
void fit_print_results(const struct fit_model *model);
void fit_model_free(struct fit_model *model);
double fit_external2(size_t n, const double x[], void *params);

#endif
