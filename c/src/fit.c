#include <assert.h>
#include <errno.h>
#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <spawn.h>              // see manpages-posix-dev
#include <poll.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <string.h>
#include <time.h>
#include <gsl/gsl_errno.h>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_randist.h>
#include <gsl/gsl_rng.h>
#include <gsl/gsl_multimin.h>
#include <useful/string.h>

#include "fit.h"

#define FIT_TABLE_SIZE 50
#define FIT_NUM_MAX_LEN 18

_Thread_local gsl_rng *rng;

// Test functions

double fit_sphere(size_t n, const double x[], void *params)
{
        double total = 0.0;
        for (size_t i = 0; i < n; i++)
                total += x[i] * x[i];
        return total;
}

double fit_rastrigin(size_t n, const double x[], void *params)
{
        double total = 10 * n;
        for (size_t i = 0; i < n; i++) {
                double term = (10.0 + x[i]) * (10.0 + x[i]) -
                    10.0 * cos(2 * M_PI * (x[i] + 10.0));
                total += term;
        }
        return total;
}

double fit_flipflop(size_t n, const double x[], void *params)
{
        double total = 15.0;
        for (size_t i = 0; i < n; i++)
                total += x[i];
        return fabs(total);
}

double fit_external(size_t n, const double x[], void *params)
{
        char *prog = params;
        char *command;
        char num_as_str[FIT_NUM_MAX_LEN];
        size_t capacity = (n + 1) * FIT_NUM_MAX_LEN + strlen(prog) + 2;

        // Put the vector into a string
        command = malloc(capacity * sizeof(*command));
        if (command == NULL) {
                fprintf(stderr,
                        "cannot allocate memory for any vector element at %s:%d\n",
                        __FILE__, __LINE__);
                return DBL_MAX;
        }
        strcpy(command, prog);
        strcat(command, " ");
        for (size_t i = 0; i < n; i++) {
                snprintf(num_as_str, FIT_NUM_MAX_LEN, "%f", x[i]);
                strcat(command, num_as_str);
                strcat(command, " ");
        }

        // call the prog using popen
        FILE *f = popen(command, "r");
        free(command);
        if (f == NULL) {
                fprintf(stderr, "cannot call external prog %s at %s:%d\n", prog,
                        __FILE__, __LINE__);
                return DBL_MAX;
        }
        if (fgets(num_as_str, FIT_NUM_MAX_LEN, f) == NULL) {
                fprintf(stderr, "problem calling %s at %s:%d\n", prog, __FILE__,
                        __LINE__);
                errno = EINVAL;
                return DBL_MAX;
        }
        double result = atof(num_as_str);
        pclose(f);
        return result;
}

double fit_external2(size_t n, const double x[], void *params)
{
        // Build array of strings
        char *command= malloc(sizeof(char) * 32 * n + n + 64);
        if (command== NULL) {
                fprintf(stderr, "cannot allocate memory for numbers\n");
                return DBL_MAX;
        }
        strncpy(command, params, 63);
        for (size_t i = 0; i < n; i++) {
                char num[32];
                snprintf(num, 31, " %f", x[i]);
                strcat(command, num);
        }
        // Set up call to posix_spawnp
        int exit_code;
        int stdout_pipe[2];
        int stderr_pipe[2];
        posix_spawn_file_actions_t action;

        if (pipe(stdout_pipe) || pipe(stderr_pipe))
                fprintf(stderr, "pipe returned an error.\n");

        posix_spawn_file_actions_init(&action);
        posix_spawn_file_actions_addclose(&action, stdout_pipe[0]);
        posix_spawn_file_actions_addclose(&action, stderr_pipe[0]);
        posix_spawn_file_actions_adddup2(&action, stdout_pipe[1], 1);
        posix_spawn_file_actions_adddup2(&action, stderr_pipe[1], 2);

        posix_spawn_file_actions_addclose(&action, stdout_pipe[1]);
        posix_spawn_file_actions_addclose(&action, stderr_pipe[1]);

        pid_t pid;
        char *argsmem[] = { "sh", "-c" };      // allows non-const access to literals
        char *args[] = { &argsmem[0][0], &argsmem[1][0], &command[0], (char *) 0};
        if (posix_spawnp(&pid, args[0], &action, NULL, &args[0], NULL) != 0) {
                fprintf(stderr, "posix_spawnp failed with error: %s\n", strerror(errno));
                return DBL_MAX;
        } else {
                errno = 0;
        }

        close(stdout_pipe[1]), close(stderr_pipe[1]);   // close child-side of pipes
        // Read from pipes
        double result = DBL_MAX;
        char buffer[1024];
        struct pollfd plist[] =
            { {stdout_pipe[0], POLLIN}, {stderr_pipe[0], POLLIN} };

        for (int rval;
             (rval = poll(&plist[0], 2, /*timeout */ -1)) > 0;) {
                if (plist[0].revents & POLLIN) {
                        read(stdout_pipe[0], &buffer[0], 1024);
                        result = atof(buffer);
                } else if (plist[1].revents & POLLIN) {
                        /* puts("F"); */
                        /* int bytes_read = */
                        /*     read(stderr_pipe[0], &buffer[0], 1024); */
                        /* printf("read %d stderr\n", bytes_read); */
                        /* printf("%s\n", buffer); */
                        /* puts("F.1"); */
                } else
                        break;  // nothing left to read
        }
        waitpid(pid, &exit_code, 0);
        posix_spawn_file_actions_destroy(&action);
        return result;
}

// String functions

char *vecf_str(size_t n, const double vec[])
{
        U_STRING(s);
        U_STRING(result);
        U_JOIN(vec, n, ", ", "%f", s);
        if (errno == 0)
                u_sprintf(&result, "[%s]", s.str);
        U_STRING_FREE(s);
        return result.str;
}

char *vecu_str(size_t n, const unsigned vec[])
{
        U_STRING(s);
        U_STRING(result);
        U_JOIN(vec, n, ", ", "%u", s);
        if (errno == 0)
                u_sprintf(&result, "[%s]", s.str);
        U_STRING_FREE(s);
        return result.str;
}

static char *lohitostr(const void *v)
{
        static _Thread_local char result[62];
        struct fit_lo_hi lohi = *(struct fit_lo_hi *)v;
        snprintf(result, 61, "(%.2f, %.2f)", lohi.lo, lohi.hi);
        return result;
}

char *vec_lo_hi_str(size_t n, const struct fit_lo_hi vec[])
{
        struct u_string s = u_join_conv(n, sizeof(struct fit_lo_hi), vec, ", ",
                                        lohitostr, false);
        return s.str;
}

static int rng_alloc(int seed)
{
        gsl_rng_env_setup();
        const gsl_rng_type *T = gsl_rng_default;
        rng = gsl_rng_alloc(T);
        if (rng == NULL) {
                fprintf(stderr, "failed to allocate rng at %s %d\n", __FILE__,
                        __LINE__);
                return -1;
        }
        gsl_rng_set(rng, seed);
        return 0;
}

struct fit_model fit_model_default()
{
        struct fit_model model;
        static struct fit_lo_hi lo_hi[] = { {-100.0, 100.0} };
        static uint32_t divisions[] = { 5 };

        model.verbose = false;
        model.variables = 1;
        strncpy(model.function_name, "external", FIT_STR_LEN);
        model.function = &fit_external2;
        model.lo_hi = lo_hi;
        model.free_lo_hi = false;
        model.divisions = divisions;
        model.free_divisions = false;
        model.generations = 4;
        model.passes = 1;
        model.threads = 1;
        model.iterations = 1000;
        model.min_error = 0.01;
        strncpy(model.method, "grid", FIT_STR_LEN);
        strncpy(model.command, "./sphere", FIT_LONG_STR_LEN);
        model.best = NULL;
        model.lowest = DBL_MAX;
        model.func_calls = 0;
        model.params = model.command;
        model.error = 0;

        return model;
}

static double exec_func(struct fit_model *model, const double x[])
{
        ++model->func_calls;
        return model->function(model->variables, x, model->params);
}

static void random_opt(struct fit_model *model)
{
        model->lowest = DBL_MAX;
        model->best = malloc(sizeof(*model->best) * model->variables);
        if (model->best == NULL) {
                model->error = ENOMEM;
                fprintf(stderr, "cannot allocate memory %s at %d\n", __FILE__,
                        __LINE__);
                return;
        }
        double *v = malloc(sizeof(*model->best) * model->variables);
        if (v == NULL) {
                model->error = ENOMEM;
                fprintf(stderr, "cannot allocate memory %s at %d\n", __FILE__,
                        __LINE__);
                return;
        }

        for (uint32_t i = 0; i < model->iterations && errno == 0; i++) {
                for (uint32_t j = 0; j < model->variables; j++) {
                        double lo = model->lo_hi->lo;
                        double hi = model->lo_hi->hi;
                        v[j] = gsl_rng_uniform(rng) * (hi - lo) + lo;
                }
                double result = exec_func(model, v);
                if (result < model->lowest) {
                        model->lowest = result;
                        memcpy(model->best, v,
                               model->variables * sizeof(double));
                        if (result < model->min_error)
                                break;
                }
        }
        free(v);
}

static void grid_opt(struct fit_model *model)
{

}

static void gsl_opt(struct fit_model *model)
{

}

void fit_model_print(const struct fit_model *model)
{
        char *lohi = vec_lo_hi_str(model->variables, model->lo_hi);
        char *div = vecu_str(model->variables, model->divisions);
        printf("Verbose: %s\n", model->verbose ? "on" : "off");
        printf("Variables: %u\n", model->variables);
        printf("Function name: %s\n", model->function_name);
        printf("Domains: %s\n", lohi);
        printf("Divisions: %s\n", div);
        printf("Generations %u\n", model->generations);
        printf("Passes: %u\n", model->passes);
        printf("Threads: %u\n", model->threads);
        printf("Iterations: %u\n", model->iterations);
        printf("Min error: %f\n", model->min_error);
        printf("Method: %s\n", model->method);
        printf("Command: %s\n", model->command);
        free(lohi);
        free(div);
}

static void set_params(struct fit_model *model)
{
        if (model->function == fit_external || model->function == fit_external2)
                model->params = (char *)model->command;
}

void fit_print_results(const struct fit_model *model)
{
        char *s = vecf_str(model->variables, model->best);
        printf("Best vector: %s\n", s);
        printf("Minimum found: %f\n", model->lowest);
        printf("Function calls: %u\n", model->func_calls);
        free(s);
}

void fit_model_free(struct fit_model *model)
{
        if (model->best) {
                free(model->best);
                model->best = NULL;
        }
        if (model->free_divisions) {
                free(model->divisions);
                model->free_divisions = false;
        }
        if (model->free_lo_hi) {
                free(model->lo_hi);
                model->free_lo_hi = false;
        }
}

void fit_optimize(struct fit_model *model)
{
        model->error = 0;
        if (rng_alloc(time(NULL)) != 0) {
                model->error = ENOMEM;
                return;
        }
        set_params(model);
        if (strcmp(model->method, "random") == 0) {
                random_opt(model);
        } else if (strcmp(model->method, "grid") == 0) {
                grid_opt(model);
        } else {
                gsl_opt(model);
        }
        gsl_rng_free(rng);
}
