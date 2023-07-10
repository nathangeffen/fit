/*
 * Class to manage optimization algorithms, and in which they
 * are implemented.
 *
 * Implementation of Grid Evolve minimization algorithm.
 * See README.md for explanation of algorithm.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <https://www.gnu.org/licenses/>.  """
 **/

#ifndef GRID_HPP
#define GRID_HPP

#include <any>
#include <atomic>
#include <cfloat>
#include <cmath>
#include <algorithm>
#include <stdexcept>
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

typedef std::function<double(const std::vector<double>, const std::string &)> opt_func;

struct opt_result {
    double lowest;
    std::vector<double> best;
    unsigned calls;
};

double sphere(const std::vector<double> &v, const std::string& s);
double external(const std::vector<double>& x_i, const std::string& command);
double rastrigin(const std::vector<double> &v, const std::string& s);
double flipflop(const std::vector<double> &v, const std::string& s);

std::vector< unsigned > make_divisions(unsigned, const std::vector<unsigned>);
std::vector< std::pair<double, double> > make_domains(
        unsigned, std::vector<double>, std::vector<double>);

struct opt_parameters {
    std::string method = "grid";
    std::string func_name = "sphere";
    opt_func func = sphere;
    int variables = 1;
    std::vector < std::pair < double, double > > domains  =  { {-100.0, 100.0} } ;
    double error = 0.1;
    std::string command = "";
    bool verbose = false;
    unsigned threads = std::thread::hardware_concurrency();
    unsigned iterations = 1000;
    std::vector<unsigned> divisions = {5};
    unsigned generations = 3;
    unsigned passes = 1;
};

class Optimization {
    public:
        Optimization(opt_parameters &p);
        double exec_func(std::vector<double> x);
        opt_result optimize();
        opt_result random();
        void single_pass(unsigned pass_no, unsigned thread_no,
                const std::vector<double> step_size,
                std::vector<std::pair<double, std::vector<double>>>& results);
        opt_result grid();

    private:
        std::string method_;
        std::atomic_uint func_calls_;
        opt_func func_;
        std::string command_;
        std::vector< std::pair<double, double> > domains_;
        std::vector< std::pair<double, double> > original_domains_;
        double error_;
        unsigned threads_;
        unsigned iterations_;
        std::vector<unsigned> divisions_ = {};
        unsigned generations_;
        unsigned passes_;
};

#endif
