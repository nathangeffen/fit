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
#include <boost/program_options.hpp>
#include "grid.hpp"

namespace po = boost::program_options;

// Process command line options


void process_options(int argc, char *argv[],
        opt_parameters & parameters)
{
    po::options_description generic(
            "Try to optimize a function");
    generic.add_options()
        ("help,h", "produce help message")
        ("verbose,v", "verbose output")
        ("variables,n", po::value<unsigned>(), "number of variables")
        ("method,m", po::value<std::string>(), "optimization method")
        ("function,f", po::value<std::string>(),
         "function to optimize")
        ("command,c", po::value<std::string>(),
         "command line for when function==external")
        ("error,e", po::value<double>(),
         "minimum error stop condition")
        ("threads,t", po::value<unsigned>(), "number of threads")
        ;

    po::options_description grid(
            "Grid evolve method");
    grid.add_options()
        ("generations,g", po::value<unsigned>(),
         "number of generations")
        ("passes,p", po::value<unsigned>(),
         "number of passes")
        ("lo,l", po::value<std::vector<double>>(),
         "lowest numbers in domains")
        ("hi", po::value<std::vector<double>>(),
         "highest numbers in domains")
        ("divisions,d", po::value<std::vector<unsigned>>(),
         "number of divisions within each grid")
        ;

     po::options_description random(
            "Random method");
    random.add_options()
        ("iterations,i", po::value<unsigned>(),
         "maximum number of iterations")
        ;

    po::options_description cmdline_options;
    cmdline_options.add(generic).add(grid).add(random);

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, cmdline_options), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << cmdline_options << "\n";
        exit(EXIT_SUCCESS);
    }

    if (vm.count("verbose")) {
        parameters.verbose = true;
    }

    if (vm.count("variables")) {
        parameters.variables = vm["variables"].as<unsigned>();
    }

    if (vm.count("method")) {
        parameters.method = vm["method"].as<std::string>();
    }

    if (vm.count("function")) {
        parameters.func_name = vm["function"].as<std::string>();
    }

    if (vm.count("command")) {
        parameters.command = vm["command"].as<std::string>();
    }

    if (vm.count("error")) {
        parameters.error = vm["error"].as<double>();
    }

    if (vm.count("generations")) {
        parameters.generations = vm["generations"].as<unsigned>();
    }

    if (vm.count("passes")) {
        parameters.passes = vm["passes"].as<unsigned>();
    }

    if (vm.count("lo")) {
        parameters.lo = vm["lo"].as<std::vector<double>>();
    }

    if (vm.count("hi")) {
        parameters.hi = vm["hi"].as<std::vector<double>>();
    }

    if (vm.count("divisions")) {
        parameters.divisions = vm["divisions"].as<std::vector<unsigned>>();
    }

    if (vm.count("iterations")) {
        parameters.iterations = vm["iterations"].as<unsigned>();
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


std::vector<double> strvectodblvec(const std::vector<std::string> &vals)
{
    std::vector<double> result(vals.size());
    for (size_t i = 0; i < vals.size(); i++) {
        result[i] = std::stod(vals[i]);
    }
    return result;
}

std::vector<unsigned> strvectounsvec(const std::vector<std::string> &vals)
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
    opt_parameters parameters;

    try {
        process_options(argc, argv, parameters);
    } catch (const std::exception& e) {
        std::cerr << "Error on command line. Try:\n" << argv[0]
            << " --help \n";
        exit(EXIT_FAILURE);
    }

    if (parameters.func_name == "sphere") {
        parameters.func = sphere;
    } else if (parameters.func_name == "rastrigin") {
        parameters.func = rastrigin;
    } else if (parameters.func_name == "flipflop") {
        parameters.func = flipflop;
    } else if (parameters.func_name == "external") {
        parameters.func = external;
    };
    try {
        make_divisions(parameters);
        make_domains(parameters);
    } catch(const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        exit(EXIT_FAILURE);
    }

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

