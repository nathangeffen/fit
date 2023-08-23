fit_sources = ['fit.c']

fitlib = shared_library('fit',
  fit_sources,
  include_directories : inc,
  dependencies : [useful_dep, gsl_dep, glib_dep],
  install : false)

cliexe = executable('fitexe', 'main.c',
  include_directories : inc,
  dependencies : useful_dep,
  link_with : fitlib)
