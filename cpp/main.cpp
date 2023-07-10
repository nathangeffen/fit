/*
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

#include <iostream>
#include <string>
#include <vector>
#include <boost/process.hpp>
#include "grid.hpp"

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

static void print_help(const std::vector<Option>& options,
        const char *prog_name,
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


void set_option(Option & option, const std::string & value)
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

void process_single_option(std::vector<Option>& options, Option& current_option, bool& option_expected,
        const std::string& arg, const std::string& prog_name, const char *prog_desc = NULL)
{
    std::string name, value_s;
    unsigned start = 0;
    if (option_expected) {
        if (arg == "-h" || arg == "--help" || arg == "help") {
            print_help(options, prog_name.c_str(), prog_desc);
            exit(0);
        }
        if (arg[0] == '-') {
            if (arg.size() > 1 && arg[1] == '-') {
                start = 2; // long option
            } else {
                start = 1; // short option
            }
        } else {
            throw std::invalid_argument("Expected option but got " + arg);
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


void process_options(int argc, char *argv[], std::vector<Option>& options,
        const char *prog_desc = NULL)
{
    std::vector<std::string> arguments;
    bool option_expected = true;
    std::string arg, name, value_s;
    Option current_option;
    process_command_options(argc, argv, arguments);
    for (auto& arg: arguments) {
        process_single_option(options, current_option, option_expected, arg, argv[0], prog_desc);
    }
    if (option_expected == false) {
        std::cerr << "Argument expected for:" << current_option.name << std::endl;
        exit(1);
    }
}

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

