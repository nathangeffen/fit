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
#include <boost/thread.hpp>

namespace bp = boost::process;


// Process command line options

struct Option {
    std::string name;
    std::string short_name;
    std::string description;
    std::any value;
    char num = '1';
    std::string default_value = "true";
};

std::ostream& operator<<(std::ostream& os, const Option& option)
{
    os << option.name << ": " << option.description;
    os << " (default: ";
    if (option.value.type() == typeid(bool *)) {
        os << *std::any_cast<bool *>(option.value);
    } else if (option.value.type() == typeid(unsigned *)) {
        os << *std::any_cast<unsigned *>(option.value);
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

std::vector<std::string>
split(const std::string& str, char delim=':')
{
    std::vector<std::string> result;
    std::string token;
    for (auto it = str.begin(); it != str.end(); it++) {
        if (*it == delim) {
            result.push_back(token);
            token = "";
        } else {
            token += *it;
        }
    }
    if (token.size() > 0) {
        result.push_back(token);
    }
    return result;
}

std::vector<double> strvectodblvec(const std::vector<std::string> vals)
{
    std::vector<double> result;
    for (auto& v: vals) {
        result.push_back(std::stod(v));
    }
    return result;
}

std::vector<unsigned> strvectounsvec(const std::vector<std::string> vals)
{
    std::vector<unsigned> result;
    for (auto& v: vals) {
        result.push_back(std::stoul(v));
    }
    return result;
}


void set_option(Option & option, std::string & value)
{
    std::cerr << option.name << " " << value << "\n";
    if (option.value.type() == typeid(double *)) {
        *(std::any_cast < double *>(option.value)) = std::stod(value);
    } else if (option.value.type() == typeid(unsigned *)) {
        *(std::any_cast < unsigned *>(option.value)) =
            std::stoul(value);
    } else if (option.value.type() == typeid(int *)) {
        *(std::any_cast < int *>(option.value)) = std::stoi(value);
    } else if (option.value.type() == typeid(bool *)) {
        *(std::any_cast < bool *>(option.value)) =
            (value == "true") ? true : false;
    } else if (option.num == '1') {
        *(std::any_cast < std::string * >(option.value)) = value;
    } else if (option.num == '+') { // multiple doubles separated by ':'
        std::vector<std::string> strarr = split(value);
        *(std::any_cast< std::vector<double> * >(option.value)) = strvectodblvec(strarr);
    } else if (option.num == '*') { // multiple unsigned separated by ':'
        std::vector<std::string> strarr = split(value);
        *(std::any_cast< std::vector<unsigned> * >(option.value)) = strvectounsvec(strarr);
    } else {
        throw std::invalid_argument("unknown type for command line option " + option.name + ": " + value);
    }
}

void process_options(int argc, char *argv[], std::vector<Option>& options,
        const char *prog_desc = NULL)
{
    std::vector<std::string> arguments;
    bool option_expected = true;
    unsigned start = 0;
    std::string arg, name, value_s;
    Option current_option;
    process_command_options(argc, argv, arguments);
    for (auto& arg: arguments) {
        if (option_expected) {
            if (arg == "-h" || arg == "--help" || arg == "help") {
                print_help(options, argv[0], prog_desc);
                exit(0);
            }
            if (arg[0] == '-') {
                if (arg.size() > 1 && arg[1] == '-') {
                    start = 2; // long option
                } else {
                    start = 1; // short option
                }
            }
            name = arg.substr(start);
            bool found = false;
            for (auto & option: options) {
                if ((start == 2 && option.name == name) ||
                        (start == 1 && option.short_name == name)) {
                    found = true;
                    if (option.num == '0') {
                        value_s = option.default_value;
                        set_option(option, value_s);
                    } else {
                        option_expected = false;
                    }
                    current_option = option;
                    break;
                }
            }
            if (!found) {
                std::cerr << "Unknown option:" << arg << std::endl;
                exit(1);
            }
        } else {
            set_option(current_option, arg);
            option_expected = true;
        }
    }
    if (option_expected == false) {
        std::cerr << "Argument expected for:" << current_option.name << std::endl;
        exit(1);
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

    template<typename T>
std::string strvecT(const std::vector<T>& v)
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

    template<typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& v)
{
    os << strvecT(v);
    return os;
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

std::ostream& operator<<(std::ostream& os,
        const std::vector<std::pair<double, double>>& v)
{
    os << strvechilo(v);
    return os;
}

std::vector< unsigned > make_divisions(unsigned n,
        const std::vector<unsigned> divisions)
{
    std::vector< unsigned > v;

    if (n == 1 && divisions.size() == 1) {
        for (unsigned i = 0; i < n; i++) {
            v = divisions;
        }
    } else if (n == 1 && divisions.size() > 1) {
        v = divisions;
    } else if (n > 1 && divisions.size() == 1) {
        for (unsigned i = 0; i < n; i++) {
            v.push_back(divisions[0]);
        }
    } else {
        throw std::invalid_argument("mismatch between divisions and variables");
    }
    return v;
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
    } else if (n == 1 && lo.size() == hi.size()) {
        for (unsigned i = 0; i < lo.size(); i++) {
            v.push_back(std::pair<double, double>{lo[i], hi[i]});
        }
    } else if (n == 1 && hi.size() == 1) {
        for (unsigned i = 0; i < lo.size(); i++) {
            v.push_back(std::pair<double, double>{lo[1], hi[0]});
        }
    } else if (n == 1 && lo.size() == 1) {
        for (unsigned i = 0; i < hi.size(); i++) {
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
            original_domains_ = domains_;
            error_ = p.error;
            threads_ = p.threads;
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
                begin[i] = std::min(domains_[i].first + (double) pass_no / passes_
                        * step_size[i], original_domains_[i].second);
            }
            std::vector<double> best(domains_.size());
            double lowest = std::numeric_limits<float>::max();
            for (size_t i = 0; i < domains_.size() && lowest > error_; i++) {
                std::vector<double> v(domains_.size());
                for (size_t j = 0; j < i; j++) {
                    v[j] = best[j];
                }
                v[i] =  begin[i];
                for (size_t j = i + 1; j < domains_.size() && lowest > error_; j++) {
                    std::uniform_real_distribution<double>
                        dist(domains_[j].first, domains_[j].second);
                    v[j] = dist(rng);
                }
                lowest = std::numeric_limits<float>::max();
                for (size_t j = 0; j < divisions_[i]; j++) {
                    double f = exec_func(v);
                    if (f < lowest) {
                        lowest = f;
                        best = v;
                    }
                    v[i] = std::min(v[i] + step_size[i], original_domains_[i].second);
                }
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
                            std::max(best_ever[i] - step_size[i], original_domains_[i].first),
                            std::min(best_ever[i] + step_size[i], original_domains_[i].second)
                        };
                    }
                }
                for (size_t i = 0; i < step_size.size(); i++) {
                    step_size[i] = (domains_[i].second - domains_[i].first) /
                        divisions_[i];
                }
                unsigned p = 0;
                while (p < passes_ && lowest_ever > error_) {
                    std::vector<std::pair<double, std::vector<double>>> results(std::min(threads_, passes_-p));
                    std::vector<std::thread> threads;
                    unsigned j = 0;
                    while (j < threads_ && p < passes_ && lowest_ever > error_) {
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
        std::vector< std::pair<double, double> > original_domains_;
        double error_;
        unsigned threads_;
        unsigned iterations_;
        std::vector<unsigned> divisions_ = {};
        unsigned generations_;
        unsigned passes_;
};

void print_parameters(const opt_parameters& parameters)
{
    std::cout << "method: " << parameters.method << "\n";
    std::cout << "function: " << parameters.func_name << "\n";
    std::cout << "command: " << parameters.command << "\n";
    std::cout << "variables: " << parameters.variables << "\n";
    std::cout << "domains: " << parameters.domains << "\n";
    std::cout << "error: " << parameters.error << "\n";
    std::cout << "verbose: " << parameters.verbose << "\n";
    std::cout << "threads: " << parameters.threads << "\n";
    std::cout << "iterations: " << parameters.iterations << "\n";
    std::cout << "divisions: " << parameters.divisions << "\n";
    std::cout << "generations: " << parameters.generations << "\n";
    std::cout << "passes: " << parameters.passes << "\n";
}

int main(int argc, char *argv[])
{
    std::vector<double> lo = {-100.0};
    std::vector<double> hi = {100.0};
    std::vector<unsigned> divisions = {5};
    opt_parameters parameters;

    // Command line options
    static std::vector<Option> options =
    {
        {
            "verbose",
            "v",
            "verbose output",
            &parameters.verbose,
            '0'
        },
        {
            "variables",
            "n",
            "number of variables",
            &parameters.variables
        },
        {
            "function",
            "f",
            "function to optimize",
            &parameters.func_name
        },
        {
            "lo",
            "l",
            "lowest number in domain",
            &lo,
            '+'
        },
        {
            "hi",
            "hi",
            "highest number in domain",
            &hi,
            '+'
        },
        {
            "divisions",
            "d",
            "number of divisions per variable",
            &divisions,
            '*'
        },
        {
            "generations",
            "g",
            "number of generations for grid method",
            &parameters.generations
        },
        {
            "passes",
            "p",
            "number of passes per generation for grid method",
            &parameters.passes
        },
        {
            "iter",
            "i",
            "number of iterations for random method",
            &parameters.iterations
        },
        {
            "error",
            "e",
            "stop if error <= this value",
            &parameters.error
        },
        {
            "method",
            "m",
            "optimization method",
            &parameters.method
        },
        {
            "command",
            "c",
            "external program to run",
            &parameters.command
        },
        {
            "threads",
            "t",
            "suggested number of parallel threads",
            &parameters.threads
        }
    };

    process_options(argc, argv, options,
            "Optimize functions using grid method");


    if (parameters.func_name == "sphere") {
        parameters.func = sphere;
    } else if (parameters.func_name == "rastrigin") {
        parameters.func = rastrigin;
    } else if (parameters.func_name == "flipflop") {
        parameters.func = flipflop;
    } else if (parameters.func_name == "external") {
        parameters.func = external;
    };
    parameters.divisions = make_divisions(parameters.variables, {divisions});
    parameters.domains = make_domains(parameters.variables, lo, hi);

    if (parameters.verbose) {
        print_parameters(parameters);
    }

    Optimization og(parameters);
    opt_result result = og.optimize();

    std::cout << "Best vector: " << result.best << "\n";
    std::cout << "Minimum found: " << result.lowest << "\n";
    std::cout << "Function calls: " << result.calls << "\n";

    return 0;
}
