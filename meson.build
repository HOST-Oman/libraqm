project(
    'raqm',
    'c',
    version: '0.10.2',
    default_options: [
        'c_std=c99',
        'cpp_std=c++11',
    ],
    meson_version: '>= 0.56.0',
)

raqm_version = meson.project_version().split('.')

cc = meson.get_compiler('c')

# FreeType version ([libtool,    actual] from its docs/VERSIONS.TXT)
#                  ([pkg-config, cmake])
freetype_version = ['24.0.18', '2.11.0']

# Set freetype as a dummy, for now
freetype = dependency('', required: false)

if cc.get_argument_syntax() == 'msvc'
    freetype = dependency(
        'freetype',
        version: '>= @0@'.format(freetype_version[1]),
        method: 'cmake',
        required: false,
    )

    if not freetype.found()
        freetype_lib = cc.find_library(
            'freetype',
            has_headers: ['freetype/freetype.h'],
            required: false,
        )
        if cc.has_function('FT_Reference_Face', dependencies: freetype_lib)
            freetype = freetype_lib
        else
            message('Your FreeType2 installation is too old, falling back...')
        endif
    endif
endif

if not freetype.found()
    freetype = dependency(
        'freetype2',
        version: '>= @0@'.format(freetype_version[0]),
        method: 'pkg-config',
        fallback: ['freetype2', 'freetype_dep'],
        default_options: [
            'png=disabled',
            'bzip2=disabled',
            'zlib=disabled',
            'harfbuzz=disabled',
        ],
    )
endif

harfbuzz = dependency(
    'harfbuzz',
    version: '>= 3.0.0',
    fallback: ['harfbuzz', 'libharfbuzz_dep'],
    default_options: [
        'freetype=enabled',
        'glib=disabled',
        'gobject=disabled',
        'cairo=disabled',
        'icu=disabled',
        'tests=disabled',
    ],
)

fribidi = dependency('', required: false)
sheenbidi = dependency('', required: false)
if get_option('sheenbidi')
    sheenbidi = dependency(
        'sheenbidi',
        fallback: ['sheenbidi', 'sheenbidi_dep'],
    )
else
    fribidi = dependency(
        'fribidi',
        version: '>= 1.0.6',
        fallback: ['fribidi', 'libfribidi_dep'],
        default_options: ['docs=false', 'tests=false'],
    )
endif

deps = [freetype, harfbuzz, fribidi, sheenbidi]

subdir('src')
subdir('tests')
if get_option('docs')
    subdir('docs')
endif

pkg = import('pkgconfig')
pkg.generate(
    filebase: meson.project_name(),
    version: meson.project_version(),
    name: meson.project_name(),
    description: 'A library for complex text layout',
    url: 'https://github.com/HOST-Oman/libraqm',
    libraries: libraqm,
)

