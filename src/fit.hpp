/*

This program is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see https://www.gnu.org/licenses/

*/

#ifndef FIT_HPP
#define FIT_HPP

#include <algorithm>
#include <atomic>
#include <cfloat>
#include <cmath>
#include <functional>
#include <iostream>
#include <limits>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <tuple>
#include <unordered_map>
extern "C" {
#include <gsl/gsl_errno.h>
#include <gsl/gsl_multimin.h>
#include <gsl/gsl_randist.h>
#include <gsl/gsl_rng.h>
#include <gsl/gsl_vector.h>
}

namespace Fit {
    typedef std::function < double (const std::vector < double >) >
        opt_func;
    typedef std::function < std::vector < double >(const std::vector <
            double >)>opt_func_dx;

    struct Result {
        double lowest;
        std::vector < double >best;
        unsigned calls;
        void print();
    };

    double sphere(const std::vector < double >&v);
    std::vector < double >sphere_dx(const std::vector < double >&v);
    double rastrigin(const std::vector < double >&v);
    double flipflop(const std::vector < double >&v);

    struct external {
        explicit external(std::string & command);
        double operator() (const std::vector < double >&x_i);
        private:
        std::string command_;
    };

    struct external_dx {
        explicit external_dx(std::string & command);
        std::vector < double >operator() (const std::vector <
                double >&x_i);
        private:
        std::string command_;
    };

    struct Parameters {
        std::string method = "grid";
        std::string func_name = "sphere";
        std::string dx_name = "";
        opt_func func = Fit::sphere;
        opt_func_dx dx = Fit::sphere_dx;
        std::string command = "";
        std::string command_dx = "";
        unsigned variables = 1;
        std::vector < double >lo = { -100.0 };
        std::vector < double >hi = { 100.0 };
        std::vector < std::pair < double, double >>domains =
        { {-100.0, 100.0} };
        double error = 0.1;
        double step_size = 0.01;
        double tol = 1e-4;
        double abstol = 1e-3;
        bool verbose = false;
        unsigned threads = std::thread::hardware_concurrency();
        unsigned iterations = 1000;
        std::vector < unsigned >divisions = { 5 };
        unsigned generations = 3;
        unsigned passes = 1;
        void print();
    };

    void make_divisions(Parameters & parameters);
    void make_domains(Parameters & parameters);

    class Optimization {
        public:
            explicit Optimization(const Parameters & p = Parameters());
            Result optimize();
            Result random();
            Result grid();
            Result nelder_mead_simplex();
            Result gradient_descent();
            Parameters parameters;

        private:
            void single_pass(unsigned pass_no, unsigned thread_no,
                    const std::vector < double >&step_size,
                    std::vector < std::pair < double,
                    std::vector < double >>>&results);
            double exec_func(const std::vector < double >&x);
            static double exec_func_gsl(const gsl_vector * v, void *params);
            static void exec_func_gsl_df(const gsl_vector * v, void *params,
                    gsl_vector * df);
            static void exec_func_gsl_combined(const gsl_vector * x,
                    void *params, double *f,
                    gsl_vector * df);
            std::string method_;
            opt_func func_;
            opt_func_dx dx_;
            std::string command_;
            std::string command_dx_;
            std::vector < std::pair < double, double >>domains_;
            std::vector < std::pair < double, double >>original_domains_;
            double error_;
            double step_size_;
            double tol_;
            double abstol_;
            unsigned threads_;
            unsigned iterations_;
            std::vector < unsigned >divisions_ = { };
            unsigned generations_;
            unsigned passes_;
            std::atomic_uint func_calls_;
    };
}
#endif
