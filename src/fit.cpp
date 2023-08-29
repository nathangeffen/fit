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
#include <algorithm>
#include <atomic>
#include <boost/process.hpp>
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

namespace bp = boost::process;

static std::random_device rd;
thread_local std::default_random_engine rng(rd());

namespace Fit {
// Test functions
double sphere(const std::vector<double> &v) {
  double total = 0.0;
  for (auto x : v)
    total += x * x;
  return total;
}

std::vector<double> sphere_dx(const std::vector<double> &v) {
  std::vector<double> result(v.size());
  for (size_t i = 0; i < v.size(); i++) {
    result[i] = 2 * v[i];
  }
  return result;
}

double rastrigin(const std::vector<double> &v) {
  double total = 10 * v.size();
  for (auto it = v.begin(); it != v.end(); it++) {
    double term =
        (10.0 + *it) * (10.0 + *it) - 10.0 * std::cos(2 * M_PI * (*it + 10.0));
    total += term;
  }
  return total;
}

double flipflop(const std::vector<double> &v) {
  double total = 15.0;
  for (auto it = v.begin(); it != v.end(); it++) {
    total += *it;
  }
  return fabs(total);
}

// If an external program is being optimized this is the
// function that must be called.
external::external(std::string &command) : command_(command){};

double external::operator()(const std::vector<double> &x_i) {
  std::stringstream args;
  bp::ipstream is;
  args << " ";
  for (auto x : x_i)
    args << x << " ";

  std::string cmd = command_ + " " + args.str();
  try {
    bp::system(cmd, bp::std_out > is);
  } catch (boost::process::process_error &e) {
    std::string s = "can't call program: " + command_;
    throw std::runtime_error(s.c_str());
  }
  double result;
  is >> result;
  return result;
};

external_dx::external_dx(std::string &command) : command_(command){};

// If an external program is being optimized  and it also has an external
// gradient function this is the function that must be called.
std::vector<double> external_dx::operator()(const std::vector<double> &x_i) {
  std::stringstream args;
  bp::ipstream is;
  args << " ";
  for (auto x : x_i)
    args << x << " ";

  std::string cmd = command_ + " " + args.str();
  try {
    bp::system(cmd, bp::std_out > is);
  } catch (boost::process::process_error &e) {
    std::string s = "can't call program: " + command_;
    throw std::runtime_error(s.c_str());
  }

  std::vector<double> result;

  double d;
  while (is >> d)
    result.push_back(d);
  return result;
};

// Mean squared error. Sometimes it makes sense to make this the function
// to be minimized.

double mse(const std::vector<double> &v) {
  const double sub_term = 0.0;
  double total = 0.0;
  for (auto x : v) {
    total += (x - sub_term) * (x - sub_term);
  }
  return total / v.size();
}

// Mean squared error derivative.
std::vector<double> mse_df(const std::vector<double> &v) {
  const double sub_term = 0.0;
  std::vector<double> result(v.size());
  for (size_t i = 0; i < v.size(); i++) {
    result[i] = -2 * (v[i] - sub_term);
  }
  return result;
}

void make_divisions(Parameters &parameters) {
  const auto elems = {(size_t)parameters.variables, parameters.divisions.size(),
                      parameters.domains.size(), parameters.lo.size(),
                      parameters.hi.size()};
  size_t n = *std::max_element(elems.begin(), elems.end());

  if (n == 1 && parameters.divisions.size() >= 1) {
    ;
  } else if (n == parameters.divisions.size()) {
    ;
  } else if (n > 1 && parameters.divisions.size() == 1) {
    std::vector<unsigned> v(n);
    for (unsigned i = 0; i < n; i++) {
      v[i] = parameters.divisions[0];
    }
    parameters.divisions = v;
  } else {
    throw std::invalid_argument("mismatch between divisions and variables");
  }
}

void make_domains(Parameters &parameters) {
  const auto elems = {(size_t)parameters.variables, parameters.divisions.size(),
                      parameters.domains.size(), parameters.lo.size(),
                      parameters.hi.size()};
  size_t n = *std::max_element(elems.begin(), elems.end());
  std::vector<std::pair<double, double>> v(n);

  if (parameters.lo.size() == 1 && parameters.hi.size() == 1) {
    for (unsigned i = 0; i < n; i++) {
      v[i] = std::pair<double, double>{parameters.lo[0], parameters.hi[0]};
    }
  } else if (parameters.lo.size() == n && parameters.hi.size() == n) {
    for (unsigned i = 0; i < n; i++) {
      v[i] = std::pair<double, double>{parameters.lo[i], parameters.hi[i]};
    }
  } else if (parameters.lo.size() == n && parameters.hi.size() == 1) {
    for (unsigned i = 0; i < n; i++) {
      v[i] = std::pair<double, double>{parameters.lo[i], parameters.hi[0]};
    }
  } else if (parameters.lo.size() == 1 && parameters.hi.size() == n) {
    for (unsigned i = 0; i < n; i++) {
      v[i] = std::pair<double, double>{parameters.lo[0], parameters.hi[i]};
    }
  } else if (n == 1 && parameters.lo.size() == parameters.hi.size()) {
    for (unsigned i = 0; i < parameters.lo.size(); i++) {
      v[i] = std::pair<double, double>{parameters.lo[i], parameters.hi[i]};
    }
  } else if (n == 1 && parameters.hi.size() == 1) {
    for (unsigned i = 0; i < parameters.lo.size(); i++) {
      v[i] = std::pair<double, double>{parameters.lo[i], parameters.hi[0]};
    }
  } else if (n == 1 && parameters.lo.size() == 1) {
    for (unsigned i = 0; i < parameters.hi.size(); i++) {
      v[i] = std::pair<double, double>{parameters.lo[0], parameters.hi[i]};
    }
  } else {
    throw std::invalid_argument(
        "lo and hi must either have 1 value "
        "or the same number of values as the number of variables.");
  }
  parameters.domains = v;
}

template <typename T> std::string strvecT(const std::vector<T> &v) {
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

std::string strvechilo(const std::vector<std::pair<double, double>> &v) {
  std::stringstream ss;

  ss << "[";

  for (auto it = v.begin(); it != v.end(); it++) {
    ss << "(" << it->first << ", " << it->second << ")";
    if (std::next(it) != v.end()) {
      ss << ", ";
    }
  }
  ss << "]";
  return ss.str();
}

std::ostream &operator<<(std::ostream &os,
                         const std::vector<std::pair<double, double>> &v) {
  os << strvechilo(v);
  return os;
}

template <typename T>
std::ostream &operator<<(std::ostream &os, const std::vector<T> &v) {
  os << strvecT(v);
  return os;
}

void Parameters::print() {
  std::cout << "Method: " << method << "\n";
  std::cout << "Function: " << func_name << "\n";
  std::cout << "Derivative: " << dx_name << "\n";
  std::cout << "Command: " << command << "\n";
  std::cout << "Variables: " << variables << "\n";
  std::cout << "Domains: " << domains << "\n";
  std::cout << "Verbose: " << verbose << "\n";
  std::cout << "Threads: " << threads << "\n";
  std::cout << "Iterations: " << iterations << "\n";
  if (method == "grid" || method == "random" || method == "nms") {
    std::cout << "Error: " << error << "\n";
  }
  if (method == "grid") {
    std::cout << "Divisions: " << divisions << "\n";
    std::cout << "Generations: " << generations << "\n";
    std::cout << "Passes: " << passes << "\n";
  }
  if (method == "gradient") {
    std::cout << "Step size: " << step_size << "\n";
    std::cout << "Tolerance: " << tol << "\n";
    std::cout << "Absolute tolerance: " << abstol << "\n";
  }
}

Optimization::Optimization(const Parameters &p)
    : parameters(p), func_calls_(0) {

  Fit::make_divisions(parameters);
  Fit::make_domains(parameters);
  original_domains_ = parameters.domains;
  if (parameters.func_name == "external") {
    external e(parameters.command);
    parameters.func = e;
  }
  if (parameters.dx_name == "external") {
    external_dx e(parameters.command_dx);
    parameters.dx = e;
  }
  if (parameters.verbose) {
    parameters.print();
  }
}

double Optimization::exec_func(const std::vector<double> &x) {
  func_calls_++;
  return parameters.func(x);
}

Result Optimization::optimize() {
  if (parameters.check == true)
    check();
  if (parameters.method == "random") {
    return random();
  } else if (parameters.method == "grid") {
    return grid();
  } else if (parameters.method == "nms") {
    return nelder_mead_simplex();
  } else if (parameters.method == "gradient") {
    return gradient_descent();
  } else {
    std::string msg = "unknown optimization method " + parameters.method;
    throw std::invalid_argument(msg);
  }
}

void Result::print() {
  std::cout << "Best vector: " << best << "\n";
  std::cout << "Minimum found: " << lowest << "\n";
  std::cout << "Function calls: " << calls << "\n";
}

Result Optimization::random() {
  bool lowest_found = false;
  double lowest = std::numeric_limits<float>::max();
  std::vector<double> best;
  for (unsigned i = 0; i < parameters.iterations && lowest_found == false;
       i++) {
    std::vector<double> v(parameters.domains.size());
    for (size_t j = 0; j < parameters.domains.size(); j++) {
      std::uniform_real_distribution<double> dist(parameters.domains[j].first,
                                                  parameters.domains[j].second);
      v[j] = dist(rng);
    }
    double d = exec_func(v);
    if (d < lowest) {
      lowest = d;
      best = v;
      if (lowest < parameters.error) {
        lowest_found = true;
      }
    }
  }
  return {lowest, best, func_calls_};
}

void Optimization::single_pass(
    unsigned pass_no, unsigned thread_no, const std::vector<double> &step_size,
    std::vector<std::pair<double, std::vector<double>>> &results) {
  std::vector<double> begin(parameters.domains.size());

  for (size_t i = 0; i < parameters.domains.size(); i++) {
    begin[i] = std::min(parameters.domains[i].first +
                            (double)pass_no / parameters.passes * step_size[i],
                        original_domains_[i].second);
  }
  std::vector<double> best(parameters.domains.size());
  double lowest = std::numeric_limits<float>::max();
  for (size_t i = 0; i < parameters.domains.size() && lowest > parameters.error;
       i++) {
    std::vector<double> v(parameters.domains.size());
    for (size_t j = 0; j < i; j++) {
      v[j] = best[j];
    }
    v[i] = begin[i];
    for (size_t j = i + 1;
         j < parameters.domains.size() && lowest > parameters.error; j++) {
      std::uniform_real_distribution<double> dist(parameters.domains[j].first,
                                                  parameters.domains[j].second);
      v[j] = dist(rng);
    }
    lowest = std::numeric_limits<float>::max();
    for (size_t j = 0; j < parameters.divisions[i]; j++) {
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

Result Optimization::grid() {
  if (parameters.divisions.size() != parameters.domains.size()) {
    throw std::invalid_argument("Number of divisions must be 1 or equal to "
                                "number of domains.");
  }
  std::vector<double> best_ever(parameters.domains.size());
  double lowest_ever = std::numeric_limits<float>::max();
  std::vector<double> step_size(parameters.domains.size());

  for (unsigned g = 0;
       g < parameters.generations && lowest_ever > parameters.error; g++) {
    if (g > 0) {
      for (size_t i = 0; i < parameters.domains.size(); i++) {
        parameters.domains[i] = {
            std::max(best_ever[i] - step_size[i], original_domains_[i].first),
            std::min(best_ever[i] + step_size[i], original_domains_[i].second)};
      }
    }
    for (size_t i = 0; i < step_size.size(); i++) {
      step_size[i] =
          (parameters.domains[i].second - parameters.domains[i].first) /
          parameters.divisions[i];
    }
    unsigned p = 0;
    while (p < parameters.passes && lowest_ever > parameters.error) {
      std::vector<std::pair<double, std::vector<double>>> results(
          std::min(parameters.threads, parameters.passes - p));
      std::vector<std::thread> threads;
      unsigned j = 0;
      while (j < parameters.threads && p < parameters.passes &&
             lowest_ever > parameters.error) {
        threads.push_back(std::thread([this, p, j, step_size, &results] {
          single_pass(p, j, step_size, results);
        }));
        j++;
        p++;
      }
      for (auto &t : threads) {
        t.join();
      }
      for (auto &r : results) {
        if (r.first < lowest_ever) {
          lowest_ever = r.first;
          best_ever = r.second;
        }
      }
    }
  }
  return {lowest_ever, best_ever, func_calls_};
}

double Optimization::exec_func_gsl(const gsl_vector *v, void *params) {
  std::vector<double> v_copy(v->data, v->data + v->size);
  Optimization *o = (Optimization *)params;
  return o->exec_func(v_copy);
}
void Optimization::exec_func_gsl_df(const gsl_vector *v, void *params,
                                    gsl_vector *df) {
  std::vector<double> v_copy(v->data, v->data + v->size);
  Optimization *o = (Optimization *)params;
  auto df_vec = o->parameters.dx(v_copy);
  for (size_t i = 0; i < df_vec.size(); i++) {
    gsl_vector_set(df, i, df_vec[i]);
  }
}

void Optimization::exec_func_gsl_combined(const gsl_vector *x, void *params,
                                          double *f, gsl_vector *df) {
  *f = exec_func_gsl(x, params);
  exec_func_gsl_df(x, params, df);
}

Result Optimization::nelder_mead_simplex() {
  const gsl_multimin_fminimizer_type *T = gsl_multimin_fminimizer_nmsimplex2;
  gsl_multimin_fminimizer *s = NULL;
  gsl_vector *ss, *x;
  gsl_multimin_function minex_func;

  size_t iter = 0;
  int status;

  /* Starting point */
  x = gsl_vector_alloc(parameters.domains.size());
  for (size_t i = 0; i < parameters.domains.size(); i++) {
    std::uniform_real_distribution<double> dist(parameters.domains[i].first,
                                                parameters.domains[i].second);
    gsl_vector_set(x, i, dist(rng));
  }

  /* Set initial step sizes to 1 */
  ss = gsl_vector_alloc(parameters.domains.size());
  gsl_vector_set_all(ss, 1.0);

  /* Initialize method and iterate */
  minex_func.n = parameters.domains.size();
  minex_func.f = exec_func_gsl;
  minex_func.params = this;

  s = gsl_multimin_fminimizer_alloc(T, parameters.domains.size());
  gsl_multimin_fminimizer_set(s, &minex_func, x, ss);

  do {
    iter++;
    status = gsl_multimin_fminimizer_iterate(s);

    if (status)
      break;

    double size = gsl_multimin_fminimizer_size(s);
    status = gsl_multimin_test_size(size, parameters.error);
  } while (status == GSL_CONTINUE && iter < parameters.iterations);

  gsl_vector_free(x);
  gsl_vector_free(ss);
  double lowest = s->fval;
  std::vector<double> best(s->x->data, s->x->data + s->x->size);
  gsl_multimin_fminimizer_free(s);
  return {lowest, best, func_calls_};
}

Result Optimization::gradient_descent() {
  const gsl_multimin_fdfminimizer_type *T;
  gsl_multimin_fdfminimizer *s;
  gsl_vector *x;
  gsl_multimin_function_fdf min_gsl;

  size_t iter = 0;

  min_gsl.n = parameters.domains.size();
  min_gsl.f = exec_func_gsl;
  min_gsl.df = exec_func_gsl_df;
  min_gsl.fdf = exec_func_gsl_combined;
  min_gsl.params = this;

  x = gsl_vector_alloc(parameters.domains.size());
  for (size_t i = 0; i < parameters.domains.size(); i++) {
    std::uniform_real_distribution<double> dist(parameters.domains[i].first,
                                                parameters.domains[i].second);
    gsl_vector_set(x, i, dist(rng));
  }

  T = gsl_multimin_fdfminimizer_conjugate_fr;
  s = gsl_multimin_fdfminimizer_alloc(T, parameters.domains.size());

  gsl_multimin_fdfminimizer_set(s, &min_gsl, x, parameters.step_size,
                                parameters.tol);

  int status;
  do {
    iter++;
    status = gsl_multimin_fdfminimizer_iterate(s);

    if (status)
      break;

    status = gsl_multimin_test_gradient(s->gradient, parameters.abstol);
  } while (status == GSL_CONTINUE && iter < parameters.iterations);

  gsl_vector_free(x);
  double lowest = s->f;
  std::vector<double> best(s->x->data, s->x->data + s->x->size);
  gsl_multimin_fdfminimizer_free(s);
  return {lowest, best, func_calls_};
}

void Optimization::check()
{
// Very basic checks currently. This can be improved.

    if (parameters.func == NULL)
        throw std::invalid_argument("function to optimize must be set");
    if (parameters.func_name == "external" && parameters.command == "")
        throw std::invalid_argument("command parameter must be set if function is external");
    if (parameters.dx_name == "external" && parameters.command_dx == "")
        throw std::invalid_argument("command_dx parameter must be set if dx is external");
    if (parameters.method == "gradient") {
        if (parameters.dx == NULL)
            throw std::invalid_argument("differential function for gradient descent must be set");
    }
}

} // namespace Fit
