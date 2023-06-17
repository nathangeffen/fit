#include <iostream>
#include <limits>
#include <numbers>
#include <random>
#include <vector>
#include <functional>

thread_local std::random_device rd;
thread_local std::mt19937 gen(rd());

// Test functions

double sphere(const std::vector<double>& x_i)
{
    double total = 0.0;

    for (auto x: x_i)
        total += x * x;

    return total;
}

double rastrigin(const std::vector<double>& x_i)
{
    double total = 0.0;

    for (auto x: x_i)
        total += x * x - 10 * cos(2 * std::numbers::pi * x);
    total += 10 * x_i.size();

    return total;
}

////////////////////////////

struct Options {
    int iterations = 100000;
};

std::ostream& operator<< (std::ostream& out, const std::vector<double>& v) {
    out << '[';
    if ( !v.empty() ) {
        for (size_t i = 0; i < v.size() - 1; i++)
            out << v[i] << " ";
        out << v.back();
    }
    out << ']';
    return out;
}

std::vector<double>
randomize_from_ranges(const std::vector<std::pair<double, double>> & ranges)
{
    std::vector<double> result;
    for (size_t i = 0; i < ranges.size(); i++) {
        std::uniform_real_distribution<double> dist(ranges[i].first, ranges[i].second);
        result.push_back(dist(gen));
    }
    return result;
}

std::vector<double> fit(
        // Function to fit/optimize/calibrate
        std::function<double(std::vector<double>&)> func,
        // Vector of pairs of the upper and lower bounds
        // of each parameter
        const std::vector<std::pair<double, double>> & ranges,
        Options options)
{
    double best_value = std::numeric_limits<double>::max();
    std::vector<double> best_parameters;
    std::vector<double> parameters = randomize_from_ranges(ranges);
    for (int i = 0; i < options.iterations; i++) {
        double val = func(parameters);
        if (val < best_value) {
            best_value = val;
            best_parameters = parameters;
        }
        parameters = randomize_from_ranges(ranges);
    }
    return best_parameters;
}

int main(int argc, char *argv[])
{
    Options options;
    options.iterations = 100000000;
    auto result = fit(sphere, {
        {-100000, 100000},
        {-100000, 100000},
        {-100000, 100000},
        {-100000, 100000},
        {-100000, 100000},
        {-10, 10},
        {-100000, 100000}
    }, options);
    std::cout << result << "\n";
    // for (auto x: result) std::cout << x << " ";
    // std::cout << "\n";
    return 0;
}
