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
sphereexe = executable('sphere', 'sphere.c')

install_headers('fit.cpp')
