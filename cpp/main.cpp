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

