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

#include <cfloat>
#include <getopt.h>
#include <cmath>
#include <algorithm>
#include <functional>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

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

double external(std::vector<double> &v, std::string& command)
{
    return 0.0;
}

struct Domain {
    double lo;
    double hi;
    std::string str() {
        std::stringstream ss;
        ss << "(" << lo << ", " << hi << ")";
        return ss.str();
    }
};

std::vector<Domain> make_domains(unsigned n, double lo, double hi)
{
    std::vector<Domain> result;
    for (unsigned i = 0; i < n; i++) {
        result.push_back({lo, hi});
    }
    return result;
}

class Counter {
    public:
        Counter(std::vector<unsigned> max_vals) {
            max_vals_ = max_vals;
            for (unsigned i = 0; i < max_vals_.size(); i++) {
                counters_.push_back(0);
            }
        }

        void inc() {
            for (int i = counters_.size() - 1; i >= 0; i--) {
                ++counters_[i];
                if (counters_[i] >= max_vals_[i]) {
                    counters_[i] = 0;
                } else {
                    break;
                }
            }
        }

        void reset() {
            std::fill(counters_.begin(), counters_.end(), 0);
        }

        inline std::vector<unsigned> get() {
            return counters_;
        }

        std::string str() {
            std::stringstream ss;
            for (auto c: counters_) {
                ss << c << " ";
            }
            return ss.str();
        }


    private:
        std::vector<unsigned> max_vals_;
        std::vector<unsigned> counters_;
};

struct Result {
    double lowest;
    std::vector<double> best;
    std::string str() {
        std::stringstream ss;
        ss << lowest << "; [ ";
        for (auto d: best) {
            ss << d << " ";
        }
        ss << " ]";
        return ss.str();
    }
};

Result grid_method(std::function<double(std::vector<double>&)> func,
        std::vector<Domain>& domains,
        unsigned iterations)
{
    unsigned vals_per_domain = (unsigned) round(pow(iterations, 1.0 / domains.size()));
    unsigned iters = (unsigned) pow(vals_per_domain, domains.size());
    auto counter = Counter(std::vector<unsigned>(domains.size(), vals_per_domain));
    auto lowest = DBL_MAX;
    std::vector<double> best;
    std::vector<double> step_size(domains.size());
    std::vector<double> begin(domains.size());

    for (unsigned i = 0; i < domains.size(); i++) {
        step_size[i] = (domains[i].hi - domains[i].lo) / (double) vals_per_domain;
        begin[i] = domains[i].lo + 0.5 * step_size[i];
    }

    for (unsigned i = 0; i < iters; i++) {
        std::vector<double> v;
        for (unsigned j = 0; j < domains.size(); j++) {
            auto val = begin[j] + counter.get()[j] * step_size[j];
            v.push_back(val);
        }
        auto r = func(v);
        if (r < lowest) {
            lowest = r;
            best = v;
        }
        counter.inc();
    }
    return {lowest, best};
}



int main(int argc, char *argv[])
{
    unsigned n = 5, iterations = 100000;
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
                case 'i': iterations = (unsigned) std::stoul(optarg);
                          break;
                case '?': break;
                default: std::cout << "Unknown option: " << c << "\n";
            }
        } catch (std::invalid_argument &e) {
           	std::cerr << "Invalid argument for " << (char) c << ": " << optarg << '\n';
            exit(EXIT_FAILURE);
        }
    }
    auto domains = make_domains(n, lo, hi);
    auto result = grid_method(sphere, domains, iterations);
    std::cout << result.str() << "\n";
    return 0;
}
