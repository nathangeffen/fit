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

#include "fit.hpp"
#include <boost/program_options.hpp>
#include <iostream>
#include <string>
#include <vector>

namespace po = boost::program_options;

// Process command line options

void process_options(int argc, char *argv[], Fit::Parameters &parameters) {
    po::options_description generic("Try to optimize a function");
    generic.add_options()("help,h", "produce help message")(
            "verbose,v", "verbose output")("variables,n", po::value<unsigned>(),
                "number of variables")(
                    "method,m", po::value<std::string>(), "optimization method")(
                    "function,f", po::value<std::string>(),
                    "function to optimize")("command,c", po::value<std::string>(),
                        "command line for when function==external")(
                            "error,e", po::value<double>(), "minimum error stop condition")(
                            "threads,t", po::value<unsigned>(), "number of threads")
                            ("check", po::value<bool>(),
                             "check that parameters are sensible before optimizing");

    po::options_description grid("Grid evolve method");
    grid.add_options()("generations,g", po::value<unsigned>(),
            "number of generations")("passes,p", po::value<unsigned>(),
                "number of passes")(
                    "lo,l", po::value<std::vector<double>>(), "lowest numbers in domains")(
                    "hi", po::value<std::vector<double>>(), "highest numbers in domains")(
                    "divisions,d", po::value<std::vector<unsigned>>(),
                    "number of divisions within each grid");

    po::options_description random("Nelder Mead and Random methods");
    random.add_options()("iterations,i", po::value<unsigned>(),
            "maximum number of iterations");

    po::options_description grad("Gradient descent method");
    grad.add_options()(
            "dx,x", po::value<std::string>(),
            "function to calculate the derivative of the function being optimized")(
                "command_dx,y", po::value<std::string>(),
                "command line for when dx==external")(
                    "step", po::value<double>(),
                    "GNU Scientific Library step size for gradient descent")(
                        "tol", po::value<double>(),
                        "GNU Scientific Library tolerance about minima for gradient descent")(
                            "abstol", po::value<double>(),
                            "GNU Scientitic Library absolute tolerance about minima for gradient "
                            "descent");

    po::options_description cmdline_options;
    cmdline_options.add(generic).add(grid).add(random).add(grad);

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, cmdline_options), vm);
    po::notify(vm);

    if (vm.count("help") || argc == 1) {
        std::cout << cmdline_options << "\n";
        exit(EXIT_SUCCESS);
    }

    if (vm.count("verbose")) {
        parameters.verbose = true;
    }

    if (vm.count("check")) {
        parameters.check = vm["check"].as<bool>();
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

    if (vm.count("command_dx")) {
        parameters.command_dx = vm["command_dx"].as<std::string>();
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

    if (vm.count("dx")) {
        parameters.dx_name = vm["dx"].as<std::string>();
    }

    if (vm.count("step")) {
        parameters.step_size = vm["step"].as<double>();
    }

    if (vm.count("tol")) {
        parameters.tol = vm["tol"].as<double>();
    }

    if (vm.count("abstol")) {
        parameters.abstol = vm["abstol"].as<double>();
    }

    if (vm.count("threads")) {
        parameters.threads = vm["threads"].as<unsigned>();
    }
}

std::vector<std::string> split(const std::string &str, char delim = ':') {
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

std::vector<double> strvectodblvec(const std::vector<std::string> &vals) {
    std::vector<double> result(vals.size());
    for (size_t i = 0; i < vals.size(); i++) {
        result[i] = std::stod(vals[i]);
    }
    return result;
}

std::vector<unsigned> strvectounsvec(const std::vector<std::string> &vals) {
    std::vector<unsigned> result;
    for (auto &v : vals) {
        result.push_back(std::stoul(v));
    }
    return result;
}

int main(int argc, char *argv[]) {
    Fit::Parameters parameters;

    try {
        process_options(argc, argv, parameters);
    } catch (const std::exception &e) {
        std::cerr << "Error on command line: " << e.what() << "\n";
        std::cerr << "Try:\n" << argv[0] << " --help \n";
        exit(EXIT_FAILURE);
    }

    if (parameters.func_name == "sphere") {
        parameters.func = Fit::sphere;
    } else if (parameters.func_name == "rastrigin") {
        parameters.func = Fit::rastrigin;
    } else if (parameters.func_name == "flipflop") {
        parameters.func = Fit::flipflop;
    }

    if (parameters.dx_name == "sphere_dx") {
        parameters.dx = Fit::sphere_dx;
    }

    try {
        Fit::Optimization og(parameters);
        Fit::Result result = og.optimize();
        result.print();
    } catch (const std::invalid_argument &e) {
        std::cerr << "Error with command line arguments: " << e.what() << "\n";
        std::cerr << "Try:\n" << argv[0] << " -h\n" << "for help.\n";
        exit(EXIT_FAILURE);
    } catch (std::exception &e) {
        std::cerr << "Error: " << e.what() << "\n";
        exit(EXIT_FAILURE);
    }
    return 0;
}
