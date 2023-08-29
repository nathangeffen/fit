#define BOOST_TEST_MODULE fit_tests
#include <boost/test/included/unit_test.hpp>
#include <boost/process.hpp>
#include "fit.hpp"

namespace bp = boost::process;

static std::string find_prog(const char *prog)
{
    std::string cmd = std::string("which ") + std::string(prog);
    bp::ipstream is;
    try {
        bp::system(cmd, bp::std_out > is);
    } catch (boost::process::process_error &e) {
        return "";
    }
    std::string full_path;
    is >> full_path;
    return full_path;
}

std::string sphere_prog = find_prog("fit_sphere");
std::string sphere_dx_prog = find_prog("fit_sphere_dx");

BOOST_AUTO_TEST_CASE(test_default_parameters) {
    try {
        Fit::Optimization fit;
        auto result = fit.optimize();
        BOOST_TEST(false); // This shouldn't run
    } catch (std::invalid_argument &e) {
        BOOST_TEST(true); // This should run
    }
}

BOOST_AUTO_TEST_CASE(test_random_internal_sphere) {
    Fit::Parameters parameters;
    parameters.method = "random";
    parameters.func_name = "sphere";
    parameters.dx_name = "";
    parameters.func = Fit::sphere;
    parameters.dx = NULL;
    parameters.command = "";
    parameters.variables = 10;
    parameters.lo = {-100.0};
    parameters.hi = {100.0};
    parameters.domains = {};
    parameters.error = 0.1;
    parameters.verbose = false;
    parameters.iterations = 100;
    make_domains(parameters);
    Fit::Optimization fit(parameters);
    auto result = fit.optimize();
    BOOST_TEST(result.lowest <= 1000000.0);
    BOOST_TEST(result.lowest >= 0.0);
    BOOST_TEST(result.best.size() == 10);
    BOOST_TEST(result.calls == 100);
}

BOOST_AUTO_TEST_CASE(test_random_external_sphere) {
    if (sphere_prog > "") {
        Fit::Parameters parameters;
        parameters.method = "random";
        parameters.func_name = "external";
        parameters.dx_name = "";
        parameters.dx = NULL;
        parameters.command = sphere_prog;
        parameters.variables = 10;
        parameters.lo = {-100.0};
        parameters.hi = {100.0};
        parameters.domains = {};
        parameters.error = 0.1;
        parameters.verbose = false;
        parameters.iterations = 100;
        make_domains(parameters);
        Fit::Optimization fit(parameters);
        auto result = fit.optimize();
        BOOST_TEST(result.lowest <= 1000000.0);
        BOOST_TEST(result.lowest >= 0.0);
        BOOST_TEST(result.best.size() == 10);
        BOOST_TEST(result.calls == 100);
    } else {
        std::cerr << "Warning: sphere test program not found.\n";
    }
}

BOOST_AUTO_TEST_CASE(test_grid_internal_sphere) {
    Fit::Parameters parameters;
    parameters.method = "grid";
    parameters.func_name = "sphere";
    parameters.dx_name = "";
    parameters.func = Fit::sphere;
    parameters.dx = NULL;
    parameters.command = "";
    parameters.variables = 10;
    parameters.lo = {-100.0};
    parameters.hi = {100.0};
    parameters.domains = {};
    parameters.error = 0.1;
    parameters.verbose = false;
    parameters.iterations = 100;
    make_domains(parameters);
    make_divisions(parameters);
    Fit::Optimization fit(parameters);
    auto result = fit.optimize();
    BOOST_TEST(result.lowest <= 1000000.0);
    BOOST_TEST(result.lowest >= 0.0);
    BOOST_TEST(result.best.size() == 10);
    BOOST_TEST(result.calls > 100);
}

BOOST_AUTO_TEST_CASE(test_grid_external_sphere) {
    if (sphere_prog > "") {
        Fit::Parameters parameters;
        parameters.method = "grid";
        parameters.func_name = "external";
        parameters.dx_name = "";
        parameters.command = sphere_prog;
        parameters.variables = 10;
        parameters.lo = {-100.0};
        parameters.hi = {100.0};
        parameters.domains = {};
        parameters.error = 0.1;
        parameters.verbose = false;
        parameters.iterations = 100;
        make_domains(parameters);
        make_divisions(parameters);
        Fit::Optimization fit(parameters);
        auto result = fit.optimize();
        BOOST_TEST(result.lowest <= 1000000.0);
        BOOST_TEST(result.lowest >= 0.0);
        BOOST_TEST(result.best.size() == 10);
        BOOST_TEST(result.calls > 100);
    } else {
        std::cerr << "Warning: sphere test program not found.\n";
    }

}

BOOST_AUTO_TEST_CASE(test_nms_internal_sphere) {
    Fit::Parameters parameters;
    parameters.method = "nms";
    parameters.func_name = "sphere";
    parameters.dx_name = "";
    parameters.func = Fit::sphere;
    parameters.dx = NULL;
    parameters.command = "";
    parameters.variables = 10;
    parameters.lo = {-100.0};
    parameters.hi = {100.0};
    parameters.domains = {};
    parameters.error = 0.1;
    parameters.verbose = false;
    parameters.iterations = 100;
    make_domains(parameters);
    make_divisions(parameters);
    Fit::Optimization fit(parameters);
    auto result = fit.optimize();
    BOOST_TEST(result.lowest <= 1000000.0);
    BOOST_TEST(result.lowest >= 0.0);
    BOOST_TEST(result.best.size() == 10);
    BOOST_TEST(result.calls > 100);
}

BOOST_AUTO_TEST_CASE(test_nms_external_sphere) {
    if (sphere_prog > "") {
        Fit::Parameters parameters;
        parameters.method = "nms";
        parameters.func_name = "external";
        parameters.dx_name = "";
        parameters.command = sphere_prog;
        parameters.variables = 10;
        parameters.lo = {-100.0};
        parameters.hi = {100.0};
        parameters.domains = {};
        parameters.error = 0.1;
        parameters.verbose = false;
        parameters.iterations = 100;
        make_domains(parameters);
        make_divisions(parameters);
        Fit::Optimization fit(parameters);
        auto result = fit.optimize();
        BOOST_TEST(result.lowest <= 1000000.0);
        BOOST_TEST(result.lowest >= 0.0);
        BOOST_TEST(result.best.size() == 10);
        BOOST_TEST(result.calls > 100);
    } else {
        std::cerr << "Warning: sphere test program not found.\n";
    }

}

BOOST_AUTO_TEST_CASE(test_gradient_internal_sphere) {
    Fit::Parameters parameters;
    parameters.method = "gradient";
    parameters.func_name = "sphere";
    parameters.dx_name = "sphere_dx";
    parameters.func = Fit::sphere;
    parameters.dx = Fit::sphere_dx;
    parameters.command = "";
    parameters.variables = 10;
    parameters.lo = {-100.0};
    parameters.hi = {100.0};
    parameters.domains = {};
    parameters.error = 0.1;
    parameters.verbose = false;
    parameters.iterations = 100;
    make_domains(parameters);
    make_divisions(parameters);
    Fit::Optimization fit(parameters);
    auto result = fit.optimize();
    BOOST_TEST(result.lowest <= 1000000.0);
    BOOST_TEST(result.lowest >= 0.0);
    BOOST_TEST(result.best.size() == 10);
    BOOST_TEST(result.calls < 50);
}

BOOST_AUTO_TEST_CASE(test_gradient_external_sphere) {
    if (sphere_prog > "" && sphere_dx_prog > "") {
        Fit::Parameters parameters;
        parameters.method = "gradient";
        parameters.func_name = "external";
        parameters.dx_name = "external";
        parameters.command = sphere_prog;
        parameters.command_dx = sphere_dx_prog;
        parameters.variables = 10;
        parameters.lo = {-100.0};
        parameters.hi = {100.0};
        parameters.domains = {};
        parameters.error = 0.1;
        parameters.verbose = false;
        parameters.iterations = 100;
        make_domains(parameters);
        make_divisions(parameters);
        Fit::Optimization fit(parameters);
        auto result = fit.optimize();
        BOOST_TEST(result.lowest <= 1000000.0);
        BOOST_TEST(result.lowest >= 0.0);
        BOOST_TEST(result.best.size() == 10);
        BOOST_TEST(result.calls < 50);
    } else {
        std::cerr << "Warning: sphere test program or sphere dx test program not found.\n";
    }
}
