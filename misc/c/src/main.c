#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <useful.h>
#include "fit.h"

/* Used for storing array of doubles - used to specify lo and hi of the domain
 * for each variable. */
struct real {
        size_t len;
        size_t capacity;
        double *vals;
};

struct natural {
        size_t len;
        size_t capacity;
        unsigned *vals;
};

struct u_option {
        const char *name;
        char has_arg;
        int *flag;
        int val;
        const char *help;
        const char *arg_desc;
};

static struct u_option option_help[] = {
        {"command", required_argument, 0, 'c',
         "external program to run", "COMMAND"},
        {"divisions", required_argument, 0, 'd',
         "divisions for each variable (separated by colon)", "INT>0"},
        {"", required_argument, 0, 'e',
         "stop if error is <= this value", "ERROR"},
        {"function", required_argument, 0, 'f',
         "function to optimize", "FUNCTION"},
        {"generations", required_argument, 0, 'g',
         "generations for grid_evolve method", "INT>0"},
        {"", no_argument, 0, 'h', "print help message", ""},
        {"iterations", required_argument, 0, 'i',
         "recommended maximum number of iterations to use to optimize",
         "INT>0"},
        {"lo", required_argument, 0, 'l',
         "list of minimum values for each domain (separated by colon)", "MIN"},
        {"hi", required_argument, 0, 'H',
         "list of minimum values for each domain (separated by colon)", "MAX"},
        {"method", required_argument, 0, 'm',
         "optimization method", "METHOD"},
        {"variables", required_argument, 0, 'n',
         "number of variables", "INT>0"},
        {"passes", required_argument, 0, 'p',
         "passes per division in grid", "INT>0"},
        {"threads", required_argument, 0, 't',
         "maximum number of parallel threads to run", "INT>0"},
        {"verbose", no_argument, 0, 'v', "verbose output", ""},
        {0, 0, 0, 0, 0}
};

struct option_array {
        size_t capacity;
        size_t len;
        struct option *options;
};

static void make_options(const char *usage, const char *desc,
                         const struct u_option options[],
                         char **short_options, struct option **long_options,
                         char **help_text)
{
        struct option_array opt_arr;
        U_ARRAY(opt_arr, options);
        U_STRING(short_opt);
        U_STRING(help);
        u_sprintf(&help, "%s\n\n", usage);
        u_strcat(&help, desc);
        const struct u_option *opt = options;
        while (opt->name != NULL) {
                if (strlen(opt->name) > 0) {
                        struct option o;
                        o.name = opt->name;
                        o.has_arg = opt->has_arg;
                        o.flag = opt->flag;
                        o.val = opt->val;
                        U_ARRAY_PUSH(opt_arr, options, o);
                }
                if (opt->val) {
                        u_pushchar(&short_opt, opt->val);
                        if (opt->has_arg)
                                u_pushchar(&short_opt, ':');
                }
                if (strlen(opt->help) > 0) {
                        u_pushchar(&help, '\t');
                        if (opt->val) {
                                u_pushchar(&help, '-');
                                u_pushchar(&help, opt->val);
                                if (strlen(opt->name) > 0)
                                        u_pushchar(&help, ' ');
                        }
                        if (strlen(opt->name) > 0) {
                                u_strcat(&help, "--");
                                u_strcat(&help, opt->name);
                        }
                        if (strlen(opt->arg_desc) > 0) {
                                u_pushchar(&help, '=');
                                u_strcat(&help, opt->arg_desc);
                        }
                        u_strcat(&help, "\n\t\t");
                        u_strcat(&help, opt->help);
                        u_strcat(&help, "\n\n");
                }
                ++opt;
        }
        u_pushchar(&help, '\n');
        *short_options = short_opt.str;
        *long_options = opt_arr.options;
        *help_text = help.str;
}

static void set_unsigned_array(struct natural *arr, const char *str)
{
        struct u_string_array strarr = u_string_split(str, ":");
        for (size_t i = 0; i < strarr.len; i++)
                U_ARRAY_PUSH(*arr, vals, (unsigned)atoi(strarr.strings[i].str));
        U_STRING_ARRAY_FREE(strarr);
}

static void set_double_array(struct real *arr, const char *str)
{
        struct u_string_array strarr = u_string_split(str, ":");
        for (size_t i = 0; i < strarr.len; i++)
                U_ARRAY_PUSH(*arr, vals, atof(strarr.strings[i].str));
        U_STRING_ARRAY_FREE(strarr);
}

static void make_domains(struct fit_model *model,
                         const struct real *lo, const struct real *hi)
{
        if (lo->len == 0 && hi->len == 0 && model->variables == 1) {
                ;               // Default vals will be used
        } else if (lo->len == 1 && hi->len == 1 && model->variables > 0) {
                model->lo_hi = malloc(sizeof(*model->lo_hi) * model->variables);
                if (model->lo_hi == NULL)
                        return;
                for (size_t i = 0; i < model->variables; i++) {
                        model->lo_hi[i].lo = lo->vals[0];
                        model->lo_hi[i].hi = hi->vals[0];
                }
        } else if (lo->len == model->variables && hi->len == model->variables) {
                model->lo_hi = malloc(sizeof(*model->lo_hi) * model->variables);
                if (model->lo_hi == NULL)
                        return;
                for (size_t i = 0; i < model->variables; i++) {
                        model->lo_hi[i].lo = lo->vals[i];
                        model->lo_hi[i].hi = hi->vals[i];
                }
        } else if (lo->len == model->variables && hi->len == 1) {
                model->lo_hi = malloc(sizeof(*model->lo_hi) * model->variables);
                if (model->lo_hi == NULL)
                        return;
                for (size_t i = 0; i < model->variables; i++) {
                        model->lo_hi[i].lo = lo->vals[i];
                        model->lo_hi[i].hi = hi->vals[0];
                }
        } else if (lo->len == 1 && hi->len == model->variables) {
                model->lo_hi = malloc(sizeof(*model->lo_hi) * model->variables);
                if (model->lo_hi == NULL)
                        return;
                for (size_t i = 0; i < model->variables; i++) {
                        model->lo_hi[i].lo = lo->vals[0];
                        model->lo_hi[i].hi = hi->vals[i];
                }
        } else if (lo->len == hi->len && model->variables == 1) {
                model->lo_hi = malloc(sizeof(*model->lo_hi) * lo->len);
                if (model->lo_hi == NULL)
                        return;
                for (size_t i = 0; i < model->variables; i++) {
                        model->lo_hi[i].lo = lo->vals[i];
                        model->lo_hi[i].hi = hi->vals[i];
                }
                model->variables = lo->len;
        } else {
                fprintf(stderr, "Mismatch between dimension of lo and hi "
                        "and number of variables.\n");
                errno = EINVAL;
        }
}

static void make_divisions(struct fit_model *model, struct natural *divisions)
{
        if (divisions->len == 0 && model->variables == 1) {
                ;               // default values will be used
        } else if (divisions->len == 1 && model->variables > 0) {
                model->divisions =
                    malloc(sizeof(*model->divisions) * model->variables);
                if (model->divisions == NULL)
                        return;
                for (size_t i = 0; i < model->variables; i++) {
                        model->divisions[i] = divisions->vals[0];
                }
        } else if ((divisions->len > 1 && model->variables) == 1 ||
                   (divisions->len > 1 && model->variables == divisions->len)) {
                for (size_t i = 0; i < model->variables; i++) {
                        model->divisions[i] = divisions->vals[i];
                }
                model->variables = divisions->len;
        } else {
                fprintf(stderr,
                        "Mismatch between divisions and dimension of domains.\n");
                errno = EINVAL;
        }
}

static void set_function(struct fit_model *model)
{
        if (strcmp(model->function_name, "sphere") == 0) {
                model->function = fit_sphere;
        } else if (strcmp(model->function_name, "rastrigin") == 0) {
                model->function = fit_rastrigin;
        } else if (strcmp(model->function_name, "flipflop") == 0) {
                model->function = fit_flipflop;
        } else if (strcmp(model->function_name, "external") == 0) {
                model->function = fit_external;
        } else if (strcmp(model->function_name, "external2") == 0) {
                model->function = fit_external2;
        } else {
                fprintf(stderr, "Unknown function %s\n", model->function_name);
                errno = EINVAL;
        }
}

static void process_arguments(struct fit_model *model, int argc,
                              char *argv[],
                              const struct option *long_options,
                              const char *short_options, const char *help)
{
        struct natural divisions;
        struct real lo, hi;
        U_ARRAY(lo, vals);
        U_ARRAY(hi, vals);
        U_ARRAY(divisions, vals);
        while (errno == 0) {
                int option_index = 0;
                int c = getopt_long(argc, argv, short_options,
                                    long_options, &option_index);
                if (c == -1)
                        break;
                switch (c) {
                case 'h':
                        printf("%s", help);
                        exit(EXIT_SUCCESS);
                case 'v':
                        model->verbose = true;
                        break;
                case 'n':
                        model->variables = (unsigned)atoi(optarg);
                        break;
                case 'm':
                        strncpy(model->method, optarg, FIT_STR_LEN - 1);
                        break;
                case 'f':
                        strncpy(model->function_name, optarg, FIT_STR_LEN - 1);
                        set_function(model);
                        break;
                case 'c':
                        strncpy(model->command, optarg, FIT_STR_LEN - 1);
                        break;
                case 'l':
                        set_double_array(&lo, optarg);
                        break;
                case 'H':
                        set_double_array(&hi, optarg);
                        break;
                case 'd':
                        set_unsigned_array(&divisions, optarg);
                        break;
                case 'g':
                        model->generations = (unsigned)atoi(optarg);
                        break;
                case 'p':
                        model->passes = (unsigned)atoi(optarg);
                        break;
                case 't':
                        model->threads = (unsigned)atoi(optarg);
                        break;
                case 'i':
                        model->iterations = (unsigned)atoi(optarg);
                        break;
                case 'e':
                        model->min_error = atof(optarg);
                        break;
                default:
                        fprintf(stderr, "Try '%s -h' for more information\n",
                                argv[0]);
                        exit(EXIT_FAILURE);
                }
        }
        if (optind < argc) {
                fprintf(stderr, "Unknown arguments: ");
                while (optind < argc)
                        fprintf(stderr, "%s ", argv[optind++]);
                printf("\n");
                fprintf(stderr, "Try '%s -h' for more information\n", argv[0]);
                exit(EXIT_FAILURE);
        }

        if (errno == 0)
                make_domains(model, &lo, &hi);
        if (errno == 0)
                make_divisions(model, &divisions);

        if (model->free_divisions == false)
                U_ARRAY_FREE(divisions, vals);
        U_ARRAY_FREE(lo, vals);
        U_ARRAY_FREE(hi, vals);
}

int main(int argc, char *argv[])
{
        errno = 0;              // If errno is set we have an error

        /* Prepare help messages and process command line options. */
        char *desc = "This utility finds a good fit for a function.\n"
            "For example, use it to find an optimal or, at least a good, "
            "set of parameters to calibrate a model.\n\n";
        U_STRING(usage);
        if (errno) {
                fprintf(stderr, "%s\n", strerror(errno));
                exit(EXIT_FAILURE);
        }
        u_sprintf(&usage, "usage: %s [options]\n", argv[0]);
        if (errno) {
                fprintf(stderr, "%s\n", strerror(errno));
                exit(EXIT_FAILURE);
        }
        char *short_options, *help;
        struct option *long_options;
        make_options(usage.str, desc, option_help, &short_options,
                     &long_options, &help);
        if (errno) {
                fprintf(stderr, "%s\n", strerror(errno));
                exit(EXIT_FAILURE);
        }
        // We initialize the model to default parameters before
        // processing the arguments.
        struct fit_model model = fit_model_default();
        process_arguments(&model, argc, argv, long_options, short_options,
                          help);
        if (errno) {
                fprintf(stderr, "%s\n", strerror(errno));
                exit(EXIT_FAILURE);
        }
        U_STRING_FREE(usage);
        free(short_options);
        free(long_options);
        free(help);
        if (model.verbose)
                fit_model_print(&model);
        // The model has been initialised and we call the optimizer.
        fit_optimize(&model);
        if (errno) {
                fprintf(stderr, "%s\n", strerror(errno));
                exit(EXIT_FAILURE);
        }
        fit_print_results(&model);
        fit_model_free(&model);
        return 0;
}
