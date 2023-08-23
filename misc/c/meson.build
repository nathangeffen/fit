project('fit', 'c',
  version : '0.1.0',
  # default_options : ['c_std=c2x'],
  license : 'GPL-2.0-or-later')

useful_dep = dependency('useful')
gsl_dep = dependency('gsl')
glib_dep = dependency('glib-2.0')

inc = include_directories('include')

subdir('include')
subdir('src')
subdir('test')

pkg_mod = import('pkgconfig')
pkg_mod.generate(libraries : fitlib,
  version : '0.1',
  name : 'libfit',
  filebase : 'fit',
  description : 'A library to calibrate models and fit or optimize functions')

