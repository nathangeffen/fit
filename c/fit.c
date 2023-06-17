#include <assert.h>
#include <errno.h>
#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <gsl/gsl_errno.h>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_randist.h>
#include <gsl/gsl_rng.h>
#include <gsl/gsl_multimin.h>

#define FIT_TABLE_SIZE 50

_Thread_local gsl_rng *rng;

// Test functions

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

double fit_rastrigin(const gsl_vector * x, void *params)
{
    double total = 0.0;
    double *p = (double *) params;
    double p_0 = p[0], p_1 = p[1];
    for (int i = 0; i < x->size; i++) {
        double d = gsl_vector_get(x, i);
        total += d * d - p_0 * cos(p_1 * M_PI * d);
    }
    return total;
}

////

typedef double (*FitFunction)(const gsl_vector *, void *);

typedef enum {
    FIT_RANDOM = 0,
    FIT_GSL_MIN = 1
} FitMethod;

typedef struct {
    double lo;
    double hi;
} FitRange;

typedef struct {
    FitMethod method;
    FitFunction fx;
    FitFunction df;
    size_t size;
    unsigned iterations;
    double error;
    void *params;
    gsl_vector *result;
    FitRange *ranges;
} FitModel;

static const FitModel fit_default_model = {
    .method = FIT_GSL_MIN,
    .fx = fit_sphere,
    .df = NULL,
    .size = 3,
    .iterations = 10000,
    .error = 1e-2,
    .params = NULL,
    .result = NULL,
    .ranges = NULL,
};

typedef struct {
    size_t data_size;
    void (*init)(FitModel *, void *);
} FitMethodEntry;

static FitMethodEntry fit_table[FIT_TABLE_SIZE];

int fit_register_method(FitMethodEntry table[],
        FitMethod method,
        size_t data_size,
        void (*init)(FitModel *, void *))
{
    if (method >= 0 && method < FIT_TABLE_SIZE) {
        table[method].data_size = data_size;
        table[method].init = init;
    } else {
        GSL_ERROR("Method id outside of method table range", ERANGE);
    }
    return 0;
}

// Populate table with optimization methods

static void register_table_entries()
{
    fit_register_method(fit_table, FIT_RANDOM, 0, NULL);
    fit_register_method(fit_table, FIT_GSL_MIN, 0, NULL);
}

////
// FIT_GSL_MIN

struct fit_gsl_min_data {
    const gsl_multimin_fminimizer_type *T;

};

///

void rng_alloc(int seed)
{
    gsl_rng_env_setup();
    int s = seed * (gsl_rng_default_seed + 1);
    const gsl_rng_type *T = gsl_rng_default;
    rng = gsl_rng_alloc(T);
    gsl_rng_set(rng, s);
}

void randomize_from_ranges(const FitRange ranges[], gsl_vector * x)
{
    for (size_t i = 0; i < x->size; i++) {
        const double lo = ranges[i].lo, hi = ranges[i].hi;
        gsl_vector_set(x, i, gsl_rng_uniform(rng) * (hi - lo) + lo);
    }
}

void fit(FitModel *model)
{
    double best_value = DBL_MAX;
    size_t n = model->size;
    int status = GSL_CONTINUE;
    const gsl_multimin_fminimizer_type *T =
        gsl_multimin_fminimizer_nmsimplex2;
    gsl_multimin_fminimizer *s = NULL;
    gsl_vector *ss = NULL;
    gsl_vector *x = gsl_vector_alloc(n);
    gsl_multimin_function minex_func;
    model->result = gsl_vector_alloc(n);

    switch (model->method) {
        case FIT_RANDOM:
            break;
        case FIT_GSL_MIN:
            randomize_from_ranges(model->ranges, x);
            ss = gsl_vector_alloc(n);
            gsl_vector_set_all(ss, 1.0);
            minex_func.n = n;
            minex_func.f = model->fx;
            minex_func.params = model->params;
            s = gsl_multimin_fminimizer_alloc(T, n);
            gsl_multimin_fminimizer_set(s, &minex_func, x, ss);
            break;
    };

    for (int i = 0; i < model->iterations && status == GSL_CONTINUE; i++) {
        switch (model->method) {
            case FIT_RANDOM:
                randomize_from_ranges(model->ranges, x);
                double val = model->fx(x, model->params);
                if (val < best_value) {
                    best_value = val;
                    gsl_vector_memcpy(model->result, x);
                }
                break;
            case FIT_GSL_MIN:
                status = gsl_multimin_fminimizer_iterate(s);
                if (status == 0) {
                    double err = gsl_multimin_fminimizer_size(s);
                    status = gsl_multimin_test_size(err, model->error);
                    if (i % 100000 == 0 || status != GSL_CONTINUE) {
                        printf("iteration %d. Status %d.\n", i, status);
                        printf("Err: %f. Status: %d\n", err, status);
                        gsl_vector_fprintf(stdout, s->x, "%f");
                        puts("");
                    }
                }
                break;
        }
    }

    if (model->method == FIT_GSL_MIN) {
        gsl_vector_memcpy(model->result, s->x);
        gsl_vector_free(ss);
        gsl_multimin_fminimizer_free(s);
    }
    gsl_vector_free(x);
}

void fit_init()
{
    rng_alloc(1);
    register_table_entries();
}

int main(int argc, char *argv[])
{
    fit_init();
    FitRange ranges[] = {
        {-100000.0, 100000.0},
        {-100000.0, 100000.0},
        {-100000.0, 100000.0},
        {-100000.0, 100000.0},
        {-100000.0, 100000.0},
        {-100000.0, 100000.0},
        {-100000.0, 100000.0},
        {-100000.0, 100000.0},
        {-100000.0, 100000.0},
        {-100000.0, 100000.0},
        {-100000.0, 100000.0},
    };
    int n = sizeof(ranges) / sizeof(FitRange);
    FitModel model = fit_default_model;
    model.fx = fit_sphere;
    model.method = FIT_GSL_MIN;
    model.size = n;
    model.error = 1e-6;
    model.iterations = 10000000;
    model.ranges = ranges;
    fit(&model);
    gsl_vector_fprintf(stdout, model.result, "%f");
    puts("");
    gsl_vector_free(model.result);
    gsl_rng_free(rng);
    return 0;
}
