project(
  'lightweight-offscreen',
  'c',
  version : '0.0.1',
  license : 'CC0-1.0',
  meson_version : '>= 0.47',
  default_options : ['c_std=gnu99', 'warning_level=2'],
)

cc = meson.get_compiler('c')

dep_gbm = dependency('gbm')
dep_egl = dependency('egl')
dep_gles2 = dependency('glesv2')

executable(
  'lightweight-offscreen',
  'lightweight-offscreen.c',
  dependencies : [ dep_gbm, dep_egl, dep_gles2, ],
  install : true,
)
