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

#endif
