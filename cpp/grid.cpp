/*
 * Reference implementation of Grid Evolve minimization algorithm.
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

#include <atomic>
#include <any>
#include <cfloat>
#include <getopt.h>
#include <cmath>
#include <algorithm>
#include <stdexcept>
#include <functional>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>
#include <boost/process.hpp>

namespace bp = boost::process;

typedef std::function<double(const std::vector<double>, std::any)> opt_func;

double sphere(std::vector<double> &v)
{
    double total = 0.0;
    for (auto x: v)
        total += x * x;
    return total;
}

double rastrigin(std::vector<double> &v)
{
    double total = 10 * v.size();
    for (auto it=v.begin(); it != v.end(); it++) {
        double term = (10.0 + *it) * (10.0 + *it) -
            10.0 * std::cos(2 * M_PI * (*it + 10.0));
        total += term;
    }
    return total;
}

double external(const std::vector<double>& x_i, const std::string& command)
{
    std::stringstream args;
    bp::ipstream is;
    args << " ";
    for (auto x: x_i)
        args << x << " ";

    std::string cmd = command + " " + args.str();
    bp::system(cmd, bp::std_out > is);

    double result;
    is >> result;
    return result;
}

std::vector< std::pair<double, double> > make_domains(
        unsigned n, std::vector<double> lo, std::vector<double> hi)
{
    std::vector< std::pair<double, double> > v;

    if (lo.size() == 1 && hi.size() == 1) {
        for (unsigned i = 0; i < n; i++) {
            v.push_back(std::pair<double, double>{lo[0], hi[0]});
        }
    } else if (lo.size() == n && hi.size() == n) {
        for (unsigned i = 0; i < n; i++) {
            v.push_back(std::pair<double, double>{lo[i], hi[i]});
        }
    } else if (lo.size() == n && hi.size() == 1) {
        for (unsigned i = 0; i < n; i++) {
            v.push_back(std::pair<double, double>{lo[i], hi[0]});
        }
    } else if (lo.size() == 1 && hi.size() == n) {
        for (unsigned i = 0; i < n; i++) {
            v.push_back(std::pair<double, double>{lo[0], hi[i]});
        }
    } else {
        throw std::invalid_argument("lo and hi must either have 1 value "
                "or the same number of values as the number of variables.");
    }
    return v;
}

/* Class to manage optimization algorithms, and in which they
 * are implemented.
 */
class Optimization {
    public:
    void init(const char *method,
              opt_func func,
              std::any& user_data,
              std::vector< std::pair<double, double> >& domains,
              double error=0.1,
              const char *command=NULL,
              bool verbose=false,
              unsigned max_threads=1,
              const std::unordered_map<const char *, double>& args={}) {


    }

    private:
    std::string method_;
    std::atomic_uint func_calls = 0;
};

int main(int argc, char *argv[])
{
    unsigned n = 5; //, iterations = 100000;
    double lo = -100.0;
    double hi = 100.0;

    // Process command line
    int c;
    while (1) {
        int option_index = 0;
        static struct option long_options[] = {
            {"n", required_argument, 0,  'n'},
            {"lo", required_argument, 0,  'l'},
            {"hi", required_argument, 0,  'h'},
            {"iterations", required_argument, 0, 'i'},
            {0, 0, 0, 0}
        };

        c = getopt_long(argc, argv, "n:l:h:i:",
                long_options, &option_index);
        if (c == -1)
            break;
        try {
            switch(c) {
                case 'n': n = (unsigned) std::stoul(optarg);
                          break;
                case 'l': lo = std::stod(optarg);
                          break;
                case 'h': hi = std::stod(optarg);
                          break;
                //case 'i': iterations = (unsigned) std::stoul(optarg);
                //          break;
                case '?': break;
                default: std::cout << "Unknown option: " << c << "\n";
            }
        } catch (std::invalid_argument &e) {
            std::cerr << "Invalid argument for " << (char) c << ": " << optarg << '\n';
            exit(EXIT_FAILURE);
        }
    }
    auto domains = make_domains(n, {lo}, {hi});
    //auto result = grid_method(sphere, domains, iterations);

    return 0;
}
