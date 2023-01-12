gtkdoc = find_program('gtkdoc-scan', required: false)

if not gtkdoc.found()
    error('MESON_SKIP_TEST gtkdoc not found.')
endif

version_xml = configure_file(
    input: 'version.xml.in',
    output: 'version.xml',
    configuration: version_h,
)

gnome = import('gnome')
gnome.gtkdoc(
    'raqm',
    main_xml: 'raqm-docs.xml',
    src_dir: [
        meson.project_source_root() / 'src',
        meson.project_build_root() / 'src',
    ],
    ignore_headers: ['config.h'],
    content_files: [version_xml],
    mkdb_args: ['--xml-mode', '--output-format=xml'],
    install: true,
    scan_args: ['--ignore-decorators=RAQM_API'],
    check: true,
)
