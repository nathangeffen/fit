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

#include <algorithm>
#include <atomic>
#include <cfloat>
#include <cmath>
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
#include "fit.hpp"

namespace bp = boost::process;

static std::random_device rd;
thread_local std::default_random_engine rng(rd());

double sphere(const std::vector < double >&v)
{
    double total = 0.0;
    for (auto x:v)
        total += x * x;
    return total;
}

double rastrigin(const std::vector < double >&v)
{
    double total = 10 * v.size();
    for (auto it = v.begin(); it != v.end(); it++) {
        double term =
            (10.0 + *it) * (10.0 + *it) -
            10.0 * std::cos(2 * M_PI * (*it + 10.0));
        total += term;
    }
    return total;
}

double flipflop(const std::vector < double >&v)
{
    double total = 15.0;
    for (auto it = v.begin(); it != v.end(); it++) {
        total += *it;
    }
    return fabs(total);
}

struct external {
    explicit external(std::string & command):command_(command) {
    } double operator() (const std::vector < double >&x_i) {
        std::stringstream args;
        bp::ipstream is;
        args << " ";
        for (auto x:  x_i)
            args << x << " ";

        std::string cmd = command_ + " " + args.str();
        bp::system(cmd, bp::std_out > is);

        double result;
        is >> result;
        return result;
    }
    private:
    std::string command_;
};

void make_divisions(opt_parameters & parameters)
{
    const auto elems =
    { (size_t)parameters.variables, parameters.divisions.size(),
        parameters.domains.size(), parameters.lo.size(),
        parameters.hi.size()
    };
    size_t n = *std::max_element(elems.begin(), elems.end());

    if (n == 1 && parameters.divisions.size() >= 1) {
        ;
    } else if (n > 1 && parameters.divisions.size() == 1) {
        std::vector < unsigned >v(n);
        for (unsigned i = 0; i < n; i++) {
            v[i] = parameters.divisions[0];
        }
        parameters.divisions = v;
    } else if (n == parameters.divisions.size()) {
        ;
    } else {
        throw std::invalid_argument
            ("mismatch between divisions and variables");
    }
}

void make_domains(opt_parameters & parameters)
{
    const auto elems =
    { (size_t)parameters.variables, parameters.divisions.size(),
        parameters.domains.size(), parameters.lo.size(),
        parameters.hi.size()
    };
    size_t n = *std::max_element(elems.begin(), elems.end());
    std::vector < std::pair < double, double >>v(n);

    if (parameters.lo.size() == 1 && parameters.hi.size() == 1) {
        for (unsigned i = 0; i < n; i++) {
            v[i] = std::pair < double, double > {
                parameters.lo[0], parameters.hi[0]};
        }
    } else if (parameters.lo.size() == n && parameters.hi.size() == n) {
        for (unsigned i = 0; i < n; i++) {
            v[i] = std::pair < double, double > {
                parameters.lo[i], parameters.hi[i]};
        }
    } else if (parameters.lo.size() == n && parameters.hi.size() == 1) {
        for (unsigned i = 0; i < n; i++) {
            v[i] = std::pair < double, double > {
                parameters.lo[i], parameters.hi[0]};
        }
    } else if (parameters.lo.size() == 1 && parameters.hi.size() == n) {
        for (unsigned i = 0; i < n; i++) {
            v[i] = std::pair < double, double > {
                parameters.lo[0], parameters.hi[i]};
        }
    } else if (n == 1 && parameters.lo.size() == parameters.hi.size()) {
        for (unsigned i = 0; i < parameters.lo.size(); i++) {
            v[i] = std::pair < double, double > {
                parameters.lo[i], parameters.hi[i]};
        }
    } else if (n == 1 && parameters.hi.size() == 1) {
        for (unsigned i = 0; i < parameters.lo.size(); i++) {
            v[i] = std::pair < double, double > {
                parameters.lo[i], parameters.hi[0]};
        }
    } else if (n == 1 && parameters.lo.size() == 1) {
        for (unsigned i = 0; i < parameters.hi.size(); i++) {
            v[i] = std::pair < double, double > {
                parameters.lo[0], parameters.hi[i]};
        }
    } else {
        throw
            std::invalid_argument("lo and hi must either have 1 value "
                    "or the same number of values as the number of variables.");
    }
    parameters.domains = v;
}

Optimization::Optimization(opt_parameters & p)
    :
        method_(p.method), func_(p.func), command_(p.command), domains_(p.domains),
        original_domains_(p.domains), error_(p.error), threads_(p.threads),
        iterations_(p.iterations), divisions_(p.divisions),
        generations_(p.generations), passes_(p.passes), func_calls_(0)
{
    if (p.func_name == "external") {
        external e(command_);
        func_ = e;
    }
}

double Optimization::exec_func(const std::vector < double > &x)
{
    func_calls_++;
    return func_(x);
}

opt_result Optimization::optimize()
{
    std::cout << "Method: " << method_ << "\n";
    if (method_ == "random") {
        return random();
    } else if (method_ == "grid") {
        return grid();
    } else if (method_ == "nms") {
        return nelder_mead_simplex();
    } else {
        std::string msg = "unknown optimization method " + method_;
        throw std::invalid_argument(msg);
    }
}

opt_result Optimization::random()
{
    bool lowest_found = false;
    double lowest = std::numeric_limits < float >::max();
    std::vector < double >best;
    for (unsigned i = 0; i < iterations_ && lowest_found == false; i++) {
        std::vector < double >v(domains_.size());
        for (size_t j = 0; j < domains_.size(); j++) {
            std::uniform_real_distribution <
                double >dist(domains_[j].first, domains_[j].second);
            v[j] = dist(rng);
        }
        double d = exec_func(v);
        if (d < lowest) {
            lowest = d;
            best = v;
            if (lowest < error_) {
                lowest_found = true;
            }
        }
    }
    return {
        lowest, best, func_calls_};
}

void Optimization::single_pass(unsigned pass_no, unsigned thread_no,
        const std::vector < double >&step_size,
        std::vector < std::pair < double,
        std::vector < double >>>&results)
{
    std::vector < double >begin(domains_.size());
    for (size_t i = 0; i < domains_.size(); i++) {
        begin[i] =
            std::min(domains_[i].first +
                    (double)pass_no / passes_ * step_size[i],
                    original_domains_[i].second);
    }
    std::vector < double >best(domains_.size());
    double lowest = std::numeric_limits < float >::max();
    for (size_t i = 0; i < domains_.size() && lowest > error_; i++) {
        std::vector < double >v(domains_.size());
        for (size_t j = 0; j < i; j++) {
            v[j] = best[j];
        }
        v[i] = begin[i];
        for (size_t j = i + 1; j < domains_.size() && lowest > error_;
                j++) {
            std::uniform_real_distribution <
                double >dist(domains_[j].first, domains_[j].second);
            v[j] = dist(rng);
        }
        lowest = std::numeric_limits < float >::max();
        for (size_t j = 0; j < divisions_[i]; j++) {
            double f = exec_func(v);
            if (f < lowest) {
                lowest = f;
                best = v;
            }
            v[i] =
                std::min(v[i] + step_size[i],
                        original_domains_[i].second);
        }
    }
    results[thread_no] = { lowest, best };
}

opt_result Optimization::grid()
{
    if (divisions_.size() != domains_.size()) {
        throw std::invalid_argument
            ("Number of divisions must be 1 or equal to "
             "number of domains.");
    }
    std::vector < double >best_ever(domains_.size());
    double lowest_ever = std::numeric_limits < float >::max();
    std::vector < double >step_size(domains_.size());

    for (unsigned g = 0; g < generations_ && lowest_ever > error_; g++) {
        if (g > 0) {
            for (size_t i = 0; i < domains_.size(); i++) {
                domains_[i] = {
                    std::max(best_ever[i] - step_size[i],
                            original_domains_[i].first),
                    std::min(best_ever[i] + step_size[i],
                            original_domains_[i].second)
                };
            }
        }
        for (size_t i = 0; i < step_size.size(); i++) {
            step_size[i] =
                (domains_[i].second -
                 domains_[i].first) / divisions_[i];
        }
        unsigned p = 0;
        while (p < passes_ && lowest_ever > error_) {
            std::vector < std::pair < double,
                std::vector <
                    double >>>results(std::min(threads_, passes_ - p));
            std::vector < std::thread > threads;
            unsigned j = 0;
            while (j < threads_ && p < passes_
                    && lowest_ever > error_) {
                threads.push_back(std::thread
                        ([this, p, j, step_size,
                         &results] {
                         single_pass(p, j, step_size,
                                 results);
                         }
                        ));
                j++;
                p++;
            }
            for (auto & t:        threads) {
                t.join();
            }
            for (auto & r:        results) {
                if (r.first < lowest_ever) {
                    lowest_ever = r.first;
                    best_ever = r.second;
                }
            }
        }
    }
    return {
        lowest_ever, best_ever, func_calls_};
}

double Optimization::nms_func(const gsl_vector *v, void *params)
{
    std::vector<double>  v_copy(v->data, v->data + v->size);
    Optimization *o = (Optimization *) params;
    return o->exec_func(v_copy);
}

opt_result Optimization::nelder_mead_simplex()
{
    const gsl_multimin_fminimizer_type *T =
        gsl_multimin_fminimizer_nmsimplex2;
    gsl_multimin_fminimizer *s = NULL;
    gsl_vector *ss, *x;
    gsl_multimin_function minex_func;

    size_t iter = 0;
    int status;

    /* Starting point */
    x = gsl_vector_alloc(domains_.size());
    for (size_t i = 0; i < domains_.size(); i++) {
        std::uniform_real_distribution < double >
            dist(domains_[i].first, domains_[i].second);
        gsl_vector_set(x, i, dist(rng));
    }
    /* Set initial step sizes to 1 */
    ss = gsl_vector_alloc(domains_.size());
    gsl_vector_set_all(ss, 1.0);

    /* Initialize method and iterate */
    minex_func.n = domains_.size();
    minex_func.f = nms_func;
    //minex_func.params = &func_;
    minex_func.params = this;

    s = gsl_multimin_fminimizer_alloc(T, domains_.size());
    gsl_multimin_fminimizer_set(s, &minex_func, x, ss);

    do {
        iter++;
        status = gsl_multimin_fminimizer_iterate(s);

        if (status)
            break;

        double size = gsl_multimin_fminimizer_size(s);
        status = gsl_multimin_test_size(size, error_);
    } while (status == GSL_CONTINUE && iter < iterations_);

    gsl_vector_free(x);

    gsl_vector_free(ss);
    double lowest = s->fval;
    std::vector<double> best(s->x->data, s->x->data + s->x->size);
    gsl_multimin_fminimizer_free(s);
    return {lowest, best, func_calls_};
}
