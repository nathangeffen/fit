fit_sources = ['fit.cpp']

fitlib = shared_library('fit',
  fit_sources,
  dependencies : [gsl_dep],
  install : false)

cliexe = executable('fit',
  'main.cpp',
  dependencies : [boost_dep],
  link_with : fitlib)

# meson.get_compiler('c').find_library('m', required: false)
sphereexe = executable('fit_sphere', 'sphere.c')
# meson.get_compiler('c').find_library('m', required: false)
spheredxexe = executable('fit_sphere_dx', 'sphere_dx.c')

install_headers('fit.hpp')

# Tests

# inc = include_directories(join_paths('..', 'src'))
test_exe = executable('fittests', 'tests.cpp', # include_directories : inc,
                      link_with : fitlib)

test('API check', test_exe)

valgrind = find_program('valgrind', required : false)

if valgrind.found()
  test('Memory leak check', valgrind, args : ['--error-exitcode=1',
                                              '--leak-check=full',
                                              test_exe ])
else
  message('Valgrind not found, so not running memory leak tests')
endif

test('Scripts', script_exe)
