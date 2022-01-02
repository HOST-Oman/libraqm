python3 = import('python').find_installation()

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
  'cursor-position-1.test',
  'cursor-position-2.test',
  'cursor-position-3.test',
  'cursor-position-4.test',
 #'cursor-position-GB3.test',
  'cursor-position-GB4.test',
  'cursor-position-GB5.test',
  'cursor-position-GB8a.test',
  'cursor-position-GB9.test',
  'cursor-position-GB9a.test',
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
  'direction-ttb-2.test',
  'empty-text.test',
  'features-arabic.test',
  'features-kerning.test',
  'features-ligature.test',
  'invisible-glyph-explicit.test',
  'invisible-glyph-hidden.test',
  'invisible-glyph-space.test',
  'languages-sr-ru.test',
  'languages-sr.test',
  'multi-fonts-1.test',
  'multi-fonts-2.test',
  'scripts-backward-ltr.test',
  'scripts-backward-rtl.test',
  'scripts-backward.test',
  'scripts-common.test',
  'scripts-forward-ltr.test',
  'scripts-forward-rtl.test',
  'scripts-forward.test',
  'test-1.test',
  'test-2.test',
  'test-3.test',
  'test-4.test',
  'test-5.test',
  'xyoffset.test',
]

foreach filename : tests
  testname = filename.split('.')[0]

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
