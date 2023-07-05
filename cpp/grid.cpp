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
#include <vector>
#include <boost/process.hpp>


namespace bp = boost::process;


// Process command line options

struct Option {
    std::string name;
    std::string short_name;
    std::string description;
    std::any value;
    char num = '1';
};

std::ostream& operator<<(std::ostream& os, const Option& option)
{
    os << option.name << ": " << option.description;
    os << " (default: ";
    if (option.value.type() == typeid(bool *)) {
        os << *std::any_cast<bool *>(option.value);
    } else if (option.value.type() == typeid(int *)) {
        os << *std::any_cast<int *>(option.value);
    } else if (option.value.type() == typeid(double *)) {
        os << *std::any_cast<double *>(option.value);
    } else {
        os << *std::any_cast<std::string *>(option.value);
    }
    os << ')';
    return os;
}

void print_help(const std::vector<Option>& options, const char *prog_name,
        const char *description)
{
    std::cout << prog_name;
    if (description)
        std::cout << ": " << description;
    std::cout << std::endl;

    std::cout << "Syntax:" << std::endl;
    std::cout << prog_name << " ";
    for (auto & option: options)
        std::cout << "[-" << option.name << "=<value>] ";
    std::cout << std::endl;

    std::cout << "\tOptions:" << std::endl;
    for (auto &option: options) {

        std::cout << "\t-" << option << std::endl;
    }
}

void process_command_options(int argc, char *argv[],
        std::vector<std::string>& arguments)
{
    for (int i = 1; i < argc; i++) {
        arguments.push_back(std::string(argv[i]));
    }
}

void process_options(int argc, char *argv[], std::vector<Option>& options,
        const char *prog_desc = NULL)
{
    std::vector<std::string> arguments;

    process_command_options(argc, argv, arguments);

    size_t i = 0;
    while (i < arguments.size()) {
        std::string arg = arguments[i];
        if (arg == "-h" || arg == "--help" || arg == "help") {
            print_help(options, argv[0], prog_desc);
            exit(0);
        }

        int start = 0;
        if (arg[0] == '-') {
            if (arg[1] == '-')
                start = 2;
            else
                start = 1;
        }

        std::string name;
        std::string value_s;

        size_t p = arg.find('=');
        if (p == std::string::npos || p == arg.size() - 1) {
            if (i+1 < arguments.size()) {
                name = arguments[i].substr(start);
                ++i;
                value_s = arguments[i];
            } else {
                name = arguments[i].substr(start);
                value_s = "";
            }
        } else {
            name = arg.substr(start, p-start);
            value_s = arg.substr(p + 1);
        }

        int found = 0;
        for (auto & option: options) {
            if ((start == 2 && option.name == name) ||
                    (start == 1 && option.short_name == name)) {
                found = 1;
                if (option.num == '0' && value_s == "") {
                    ;
                } else if (option.num == '0' && value_s != "") {
                    std::cerr << "Option " << option.name << " does not take argument.\n";
                    print_help(options, argv[0], prog_desc);
                    exit(0);
                } else if (value_s == "") {
                    std::cerr << "Option " << option.name << " needs argument.\n";
                    print_help(options, argv[0], prog_desc);
                    exit(0);
                } else {
                    if (option.value.type() == typeid(double*)) {
                        *(std::any_cast<double*>(option.value)) = std::stod(value_s);
                    } else if (option.value.type() == typeid(int *)) {
                        *(std::any_cast<int*>(option.value))  = std::stoi(value_s);
                    } else if (option.value.type() == typeid(bool *)) {
                        *(std::any_cast<bool*>(option.value)) =
                            (value_s == "true") ? true : false;
                    } else {
                        *(std::any_cast<std::string*>(option.value)) = value_s;
                    }
                }
                break;
            }
        }
        if (!found) {
            std::cerr << "Unknown option:" << arg << std::endl;
            std::cerr << "Try: " << std::endl;
            std::cerr << '\t' << argv[0] << " --help" << std::endl;
            exit(1);
        }
        ++i;
        std::cout << "Parm: " << name << " " << value_s << "\n";
    }
}

//////////////////////

typedef std::function<double(const std::vector<double>, const std::string &)> opt_func;

static std::random_device rd;
thread_local std::default_random_engine rng(rd());

double sphere(const std::vector<double> &v, const std::string& s)
{
    double total = 0.0;
    for (auto x: v)
        total += x * x;
    return total;
}

double rastrigin(const std::vector<double> &v, const std::string& s)
{
    double total = 10 * v.size();
    for (auto it=v.begin(); it != v.end(); it++) {
        double term = (10.0 + *it) * (10.0 + *it) -
            10.0 * std::cos(2 * M_PI * (*it + 10.0));
        total += term;
    }
    return total;
}

double flipflop(const std::vector<double> &v, const std::string& s)
{
    double total = 15.0;
    for (auto it=v.begin(); it != v.end(); it++) {
        total += *it;
    }
    return fabs(total);
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

std::string strvecdbl(const std::vector<double>& v)
{
    std::stringstream ss;

    ss << "[";

    for (auto it = v.begin(); it != v.end(); it++) {
        ss << *it;
        if (std::next(it) != v.end()) {
            ss << ", ";
        }
    }
    ss << "]";
    return ss.str();
}

std::string strvechilo(const std::vector<std::pair<double, double>>& v)
{
    std::stringstream ss;

    ss << "[";

    for (auto it = v.begin(); it != v.end(); it++) {
        ss << "(" << it->first << ", " <<  it->second << ")";
        if (std::next(it) != v.end()) {
            ss << ", ";
        }
    }
    ss << "]";
    return ss.str();
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

struct opt_result {
    double lowest;
    std::vector<double> best;
    unsigned calls;
};

struct opt_parameters {
    std::string method = "grid";
    opt_func func = sphere;
    std::vector < std::pair < double, double > > domains  =  { {-100.0, 100.0} } ;
    double error = 0.1;
    std::string command = "";
    bool verbose = false;
    unsigned max_threads = 1;
    unsigned iterations = 1000;
    std::vector<unsigned> divisions = {5};
    unsigned generations = 3;
    unsigned passes = 1;
};

/* Class to manage optimization algorithms, and in which they
 * are implemented.
 */
class Optimization {
    public:
        Optimization(opt_parameters &p) {
            func_calls_ = 0;
            method_ = p.method;
            func_ = p.func;
            command_ = p.command;
            for (auto &d: p.domains) {
                domains_.push_back({d.first, d.second});
            }
            error_ = p.error;
            max_threads_ = p.max_threads;
            iterations_ = p.iterations;
            if (p.method == "grid") {
                if (p.divisions.size() > 1 && p.divisions.size() != p.domains.size()) {
                    throw std::invalid_argument(
                            "Number of divisions must be 1 or equal to "
                            "number of domains.");
                }
                divisions_ = p.divisions;
            }
            generations_ = p.generations;
            passes_ = p.passes;
        }

        double exec_func(std::vector<double> x) {
            func_calls_++;
            return func_(x, command_);
        }

        opt_result optimize() {
            if (method_ == "random") {
                return random();
            } else {
                return grid();
            }
        }

        opt_result random() {
            bool lowest_found = false;
            double lowest = std::numeric_limits<float>::max();
            std::vector<double> best;
            for (unsigned i = 0; i < iterations_ && lowest_found == false; i++) {
                std::vector<double> v;
                for (auto &d: domains_) {
                    std::uniform_real_distribution<double>
                        dist(d.first, d.second);
                    v.push_back(dist(rng));
                }
                double d = exec_func(v);
                if (d < lowest) {
                    lowest = d;
                    best = v;
                    if (lowest < error_) {
                        lowest_found = true;
                        break;
                    }
                }
            }
            return {
                lowest, best, func_calls_
            };
        }

        void single_pass(unsigned pass_no, unsigned thread_no,
                const std::vector<double> step_size,
                std::vector<std::pair<double, std::vector<double>>>& results) {
            std::vector<double> begin(domains_.size());
            for (size_t i = 0; i < domains_.size(); i++) {
                begin[i] = domains_[i].first + (double) pass_no / passes_
                    * step_size[i];
            }
            std::vector<double> best(domains_.size());
            double lowest = std::numeric_limits<float>::max();
            for (size_t i = 0; i < domains_.size() && lowest > error_; i++) {
                std::vector<double> v(domains_.size());
                for (size_t j = 0; j < i; j++) {
                    v[j] = best[j];
                }
                v[i] =  begin[i] ;
                for (size_t j = i + 1; j < domains_.size() && lowest > error_; j++) {
                    std::uniform_real_distribution<double>
                        dist(domains_[j].first, domains_[j].second);
                    v[j] = dist(rng);
                }
                lowest = std::numeric_limits<float>::max();
                for (size_t j = 0; j < divisions_[i]; j++) {
                    double f = exec_func(v);
                    std::cout << "L: " << lowest << " " << f << " " << strvecdbl(v) << "\n";
                    if (f < lowest) {
                        lowest = f;
                        best = v;
                    }
                    v[i] += step_size[i];
                }
                std::cout << "X " << lowest << " " << strvecdbl(best) << "\n";
            }
            results[thread_no] = {lowest, best};
        }


        opt_result grid() {
            std::vector<double> best_ever(domains_.size());
            double lowest_ever = std::numeric_limits<float>::max();
            std::vector<double> step_size(domains_.size());

            for (unsigned g = 0; g < generations_ && lowest_ever > error_; g++) {
                if (g > 0) {
                    for (size_t i = 0; i < domains_.size(); i++) {
                        domains_[i] = {
                            best_ever[i] - step_size[i],
                            best_ever[i] + step_size[i]
                        };
                    }
                }
                for (size_t i = 0; i < step_size.size(); i++) {
                    step_size[i] = (domains_[i].second - domains_[i].first) /
                        divisions_[i];
                }
                unsigned p = 0;
                while (p < passes_ && lowest_ever > error_) {
                    std::vector<std::pair<double, std::vector<double>>> results(max_threads_);
                    std::vector<std::thread> threads;
                    unsigned j = 0;
                    while (j < max_threads_ && p < passes_ && lowest_ever > error_) {
                        threads.push_back(std::thread([this, p, j, step_size, &results] {
                                    single_pass(p, j, step_size, results);
                                    }));
                        j++;
                        p++;
                    }
                    for (auto& t: threads) {
                        t.join();
                    }
                    for (auto &r: results) {
                        if (r.first < lowest_ever) {
                            lowest_ever = r.first;
                            best_ever = r.second;
                        }
                    }
                }
            }
            return {
                lowest_ever, best_ever, func_calls_
            };
        }

    private:
        std::string method_;
        std::atomic_uint func_calls_;
        opt_func func_;
        std::string command_;
        std::vector< std::pair<double, double> > domains_;
        double error_;
        unsigned max_threads_;
        unsigned iterations_;
        std::vector<unsigned> divisions_ = {};
        unsigned generations_;
        unsigned passes_;
};

int main(int argc, char *argv[])
{
    bool verbose = false;
    int variables = 0;
    std::string func = "external";
    double lo = -100.0;
    double hi = 100.0;
    int divisions = 5;
    int generations = 3;
    int passes = 1;
    int threads = 1;
    int iterations = 1000;
    double error = 0.01;
    std::string method = "grid";
    std::string command = "./sphere";

    // Command line options
    static std::vector<Option> options =
    {
        {
            "verbose",
            "v",
            "verbose output",
            &verbose,
        },
        {
            "variables",
            "n",
            "number of variables",
            &variables
        },
        {
            "function",
            "f",
            "function to optimize",
            &func,
        },
        {
            "lo",
            "l",
            "lowest number in domain",
            &lo
        },
        {
            "hi",
            "hi",
            "highest number in domain",
            &hi
        },
        {
            "divisions",
            "d",
            "number of divisions per variable",
            &divisions
        },
        {
            "generations",
            "g",
            "number of generations for grid method",
            &generations
        },
        {
            "passes",
            "p",
            "number of passes per generation for grid method",
            &passes
        },
        {
            "iter",
            "i",
            "number of iterations for random method",
            &iterations
        },
        {
            "error",
            "e",
            "stop if error <= this value",
            &error
        },
        {
            "method",
            "m",
            "optimization method",
            &method
        },
        {
            "command",
            "c",
            "external program to run",
            &error
        }
    };

    process_options(argc, argv, options,
            "Optimize functions using grid method");

    opt_parameters parameters;
    parameters.method = method;
    if (func == "sphere") {
        parameters.func = sphere;
    } else if (func == "rastrigin") {
        parameters.func = rastrigin;
    } else if (func == "flipflop") {
        parameters.func = flipflop;
    }
    for (int i = 0; i < variables; i++) {
        parameters.divisions.push_back(divisions);
    }
    parameters.domains = make_domains(variables, {lo}, {hi});
    parameters.error = error;
    parameters.verbose = verbose;
    parameters.max_threads = threads;
    parameters.iterations = iterations;
    parameters.generations = generations;
    parameters.passes = passes;


    Optimization og(parameters);
    opt_result result = og.optimize();
    std::cout << result.lowest << "\n";
    std::cout << strvecdbl(result.best) << "\n";
    std::cout << result.calls << "\n";

    std::cout << "Best vector: " << strvecdbl(result.best) << "\n";
    std::cout << "Minimum found: " << result.lowest << "\n";
    std::cout << "Function calls: " << result.calls << "\n";

    return 0;
}
