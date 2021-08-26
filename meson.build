project('raqm', 'c',
  version : '0.7.0',
  default_options : [
    'c_std=c99',
    'cpp_std=c++11',
  ],
)

raqm_version = meson.project_version().split('.')

freetype = dependency('freetype2', version : '>= 12.0.6',
                      fallback : ['freetype2', 'freetype_dep'],
                      default_options : ['png=disabled', 'bzip2=disabled',
                                         'zlib=disabled', 'harfbuzz=disabled'])
harfbuzz = dependency('harfbuzz', version : '>= 1.7.2',
                      fallback : ['harfbuzz', 'libharfbuzz_dep'],
                      default_options : ['freetype=enabled',
                                         'glib=disabled', 'gobject=disabled',
                                         'cairo=disabled', 'icu=disabled',
                                         'tests=disabled'])
fribidi  = dependency('fribidi',
                      fallback : ['fribidi', 'libfribidi_dep'],
                      default_options : ['docs=false', 'tests=false'])
deps = [freetype, harfbuzz, fribidi]

subdir('src')
subdir('tests')
if get_option('docs')
  subdir('docs')
endif

pkg = import('pkgconfig')
pkg.generate(
  filebase : meson.project_name(),
  version : meson.project_version(),
  name : meson.project_name(),
  description : 'A library for complex text layout',
  url : 'https://github.com/HOST-Oman/libraqm',
  libraries : libraqm,
)

