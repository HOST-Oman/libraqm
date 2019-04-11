project('raqm', 'c', version : '0.7.0', default_options : 'c_std=c99')

cc = meson.get_compiler('c')
pkg = import('pkgconfig')

freetype = dependency('freetype2', version : '>= 12.0.6',
                      fallback : ['freetype2', 'freetype_dep'],
                      default_options : ['png=disabled', 'bzip2=disabled',
                                         'zlib=disabled', 'harfbuzz=disabled'])
harfbuzz = dependency('harfbuzz',
                      fallback : ['harfbuzz', 'libharfbuzz_dep'],
                      default_options : ['freetype=enabled', 'ucdn=enabled',
                                         'glib=disabled', 'cairo=disabled',
                                         'fontconfig=disabled', 'icu=disabled',
                                         'tests=disabled'])
fribidi  = dependency('fribidi',
                      fallback : ['fribidi', 'libfribidi_dep'],
                      default_options : ['docs=false'])
deps = [freetype, harfbuzz, fribidi]

conf = configuration_data()
conf.set('VERSION', meson.project_version())

libraqm = library(
  'raqm',
  'src/raqm.c',
  'src/raqm.h',
  dependencies : deps,
  c_args : ['-DHAVE_CONFIG_H'],
  install : true,
)

libraqm_test = static_library(
  'raqm-test',
  'src/raqm.c',
  'src/raqm.h',
  dependencies : deps,
  c_args : ['-DRAQM_TESTING', '-DHAVE_CONFIG_H'],
  install : false,
)

pkg.generate(
  filebase : meson.project_name(),
  version : meson.project_version(),
  name : meson.project_name(),
  description : 'A library for complex text layout',
  url : 'https://github.com/HOST-Oman/libraqm',
  libraries : libraqm,
)

subdir('tests')
if get_option('docs')
  subdir('docs')
endif
