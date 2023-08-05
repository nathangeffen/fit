/**
 *  This program is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the Free
 *  Software Foundation, either version 3 of the License, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 *  more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program. If not, see <https://www.gnu.org/licenses/>.
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

typedef std::function<double(const std::vector<double>)> opt_func;

struct opt_result {
    double lowest;
    std::vector<double> best;
    unsigned calls;
};

double sphere(const std::vector<double> &v);
//double external(const std::vector<double> &x_i);
double rastrigin(const std::vector<double> &v);
double flipflop(const std::vector<double> &v);

struct opt_parameters {
    std::string method = "grid";
    std::string func_name = "sphere";
    std::string dx_name;
    opt_func func = sphere;
    opt_func dx = NULL;
    unsigned variables = 1;
    std::vector<double> lo = {-100.0};
    std::vector<double> hi = {100.0};
    std::vector<std::pair<double, double>> domains = {{-100.0, 100.0}};
    double error = 0.1;
    std::string command = "";
    bool verbose = false;
    unsigned threads = std::thread::hardware_concurrency();
    unsigned iterations = 1000;
    std::vector<unsigned> divisions = {5};
    unsigned generations = 3;
    unsigned passes = 1;
};

void make_divisions(opt_parameters &parameters);
void make_domains(opt_parameters &parameters);

class Optimization {
    public:
        explicit Optimization(opt_parameters &p);
        opt_result optimize();
        opt_result random();
        opt_result grid();
        opt_result nelder_mead_simplex();

    private:

        void
            single_pass(unsigned pass_no, unsigned thread_no,
                    const std::vector<double> &step_size,
                    std::vector<std::pair<double, std::vector<double>>> &results);
        double exec_func(const std::vector < double > &x);
        static double nms_func(const gsl_vector *v, void *params);
        std::string method_;
        opt_func func_;
        std::string command_;
        std::vector<std::pair<double, double>> domains_;
        std::vector<std::pair<double, double>> original_domains_;
        double error_;
        unsigned threads_;
        unsigned iterations_;
        std::vector<unsigned> divisions_ = {};
        unsigned generations_;
        unsigned passes_;
        std::atomic_uint func_calls_;
};

#endif
