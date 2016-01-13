AC_INIT([Raqm],
        [0.1.0],
        [https://github.com/HOST-Oman/libraqm/issues],
        [raqm],
        [https://github.com/HOST-Oman/libraqm/])

AM_INIT_AUTOMAKE([foreign color-tests -Wall -Wno-portability])
AM_SILENT_RULES([yes])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIRS([m4])

# Enable compiler warnings
AX_COMPILER_FLAGS

AC_PROG_CC

LT_INIT

PKG_PROG_PKG_CONFIG([0.20])
PKG_CHECK_MODULES(FREETYPE, freetype2, [], [])
PKG_CHECK_MODULES(HARFBUZZ, harfbuzz, [], [])
PKG_CHECK_MODULES(FRIBIDI, fribidi, [], [])
PKG_CHECK_MODULES(GLIB, glib-2.0, have_glib=true, have_glib=false)

AC_CHECK_FUNC(fribidi_reorder_runs, AC_DEFINE([HAVE_FRIBIDI_REORDER_RUNS], 1, [Have fribidi_reorder_runs]), [])

AM_CONDITIONAL(HAVE_GLIB, $have_glib)

AC_CONFIG_FILES([
    Makefile
    raqm.pc
    src/Makefile
    tests/Makefile
])

AC_OUTPUT
