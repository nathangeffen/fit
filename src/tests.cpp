#define BOOST_TEST_MODULE fit_tests
#include <boost/test/included/unit_test.hpp>
#include "fit.hpp"

const char *sphere_prog = "./src/fit_sphere";
const char *sphere_dx_prog = "./src/fit_sphere_dx";

BOOST_AUTO_TEST_CASE(test_default_parameters)
{
    Fit::Optimization fit;
    auto result = fit.optimize();
    BOOST_TEST(result.lowest <= 1.0);
    BOOST_TEST(result.lowest >= 0.0);
    BOOST_TEST(result.best.size() == 1);
    BOOST_TEST(result.calls > 0);
}

BOOST_AUTO_TEST_CASE(test_random_internal_sphere)
{
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
    BOOST_TEST(result.calls = 100);
}

BOOST_AUTO_TEST_CASE(test_random_external_sphere)
{
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
    BOOST_TEST(result.calls = 100);
}

BOOST_AUTO_TEST_CASE(test_grid_internal_sphere)
{
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
    BOOST_TEST(result.calls = 100);
}

BOOST_AUTO_TEST_CASE(test_grid_external_sphere)
{
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
    BOOST_TEST(result.calls = 100);
}


BOOST_AUTO_TEST_CASE(test_nms_internal_sphere)
{
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
    BOOST_TEST(result.calls = 100);
}

BOOST_AUTO_TEST_CASE(test_nms_external_sphere)
{
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
    BOOST_TEST(result.calls = 100);
}

BOOST_AUTO_TEST_CASE(test_gradient_internal_sphere)
{
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
    BOOST_TEST(result.calls = 100);
}

BOOST_AUTO_TEST_CASE(test_gradient_external_sphere)
{
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
    BOOST_TEST(result.calls = 100);
}


