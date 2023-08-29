project('fit', ['cpp', 'c',],
  version : '0.1.0',
  license : 'GPL-2.0-or-later')

boost_dep = dependency('boost', modules : ['program_options', ])
gsl_dep = dependency('gsl')
script_exe = find_program('fit_tests.sh')

subdir('src')

pkg_mod = import('pkgconfig')
pkg_mod.generate(libraries : fitlib,
  version : '0.1',
  name : 'libfit',
  filebase : 'fit',
  description : 'A library to calibrate models and fit or optimize functions')

