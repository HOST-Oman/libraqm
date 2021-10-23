python3 = import('python').find_installation('python3')

raqm_test = executable(
  'raqm-test',
  'raqm-test.c',
  include_directories : include_directories(['../src']),
  link_with : libraqm_test,
  dependencies : deps,
  install : false,
)

tests = [
  'buffer-flags-1.test',
  'cursor_position1.test',
  'cursor_position2.test',
  'cursor_position3.test',
  'cursor_position4.test',
  #'cursor_position_GB3.test',
  'cursor_position_GB4.test',
  'cursor_position_GB5.test',
  'cursor_position_GB8a.test',
  'cursor_position_GB9a.test',
  'cursor_position_GB9.test',
  'direction-ltr-1.test',
  'direction-ltr-2.test',
  'direction-ltr-3.test',
  'direction-ltr-4.test',
  'direction-ltr-5.test',
  'direction-rtl-1.test',
  'direction-rtl-2.test',
  'direction-rtl-3.test',
  'direction-rtl-4.test',
  'direction-rtl-5.test',
  'direction-ttb-1.test',
  'empty-text.test',
  'features-arabic.test',
  'features-kerning.test',
  'features-ligature.test',
  'invisible_glyph_explicit.test',
  'invisible_glyph_hidden.test',
  'invisible_glyph_space.test',
  'languages-sr-ru.test',
  'languages-sr.test',
  'multi-fonts2.test',
  'multi-fonts.test',
  'scripts-backward-ltr.test',
  'scripts-backward-rtl.test',
  'scripts-backward.test',
  'scripts-common.test',
  'scripts-forward-ltr.test',
  'scripts-forward-rtl.test',
  'scripts-forward.test',
  'test1.test',
  'test2.test',
  'test3.test',
  'test4.test',
  'test5.test',
  'xyoffset.test',
]

foreach filename : tests
  testname = filename.split('.')[0].underscorify()

  test(testname,
    python3,
    args : [
      files('runtest.py'),
      meson.current_source_dir(),
      raqm_test,
      join_paths(meson.current_source_dir(), filename),
    ],
  )
endforeach
