AC_INIT([Raqm],
        [0.7.1],
        [https://github.com/HOST-Oman/libraqm/issues],
        [raqm],
        [https://github.com/HOST-Oman/libraqm/])

AM_INIT_AUTOMAKE([foreign color-tests -Wall -Wno-portability])
AM_SILENT_RULES([yes])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

AC_PROG_CC
AC_PROG_CC_C99

# Enable compiler warnings
AX_COMPILER_FLAGS

LT_INIT

# Version
m4_define([_triplet], m4_split(AC_PACKAGE_VERSION, [[.]]))
m4_define([_major],   m4_argn(1, _triplet))
m4_define([_minor],   m4_argn(2, _triplet))
m4_define([_micro],   m4_argn(3, _triplet))
RAQM_VERSION_MAJOR=_major
RAQM_VERSION_MINOR=_minor
RAQM_VERSION_MICRO=_micro
RAQM_VERSION=AC_PACKAGE_VERSION
AC_SUBST(RAQM_VERSION_MAJOR)
AC_SUBST(RAQM_VERSION_MINOR)
AC_SUBST(RAQM_VERSION_MICRO)
AC_SUBST(RAQM_VERSION)

# Libtool version
m4_define([_age],     m4_eval(_major*10000 + _minor*100))
m4_define([_current], m4_eval(_major + _age))
RAQM_LIBTOOL_VERSION=_current:_micro:_age
AC_SUBST(RAQM_LIBTOOL_VERSION)


PKG_PROG_PKG_CONFIG([0.20])
AX_PKG_CHECK_MODULES(FREETYPE, [freetype2 >= 12.0.6], [])
AX_PKG_CHECK_MODULES(HARFBUZZ, [], harfbuzz >= 1.7.2)
AX_PKG_CHECK_MODULES(FRIBIDI, [], fribidi)

_save_libs="$LIBS"
_save_cflags="$CFLAGS"
LIBS="$LIBS $HARFBUZZ_LIBS $FRIBIDI_LIBS"
CFLAGS="$CFLAGS $HARFBUZZ_CFLAGS $FRIBIDI_CFLAGS"
AC_CHECK_FUNCS(hb_buffer_set_invisible_glyph)
AC_CHECK_DECLS([HB_BUFFER_FLAG_REMOVE_DEFAULT_IGNORABLES],,,[[#include <hb.h>]])

LIBS="$_save_libs"
CFLAGS="$_save_cflags"

GTK_DOC_CHECK([1.14],[--flavour no-tmpl])

case $build_os in
    mingw*)
        AX_APPEND_FLAG([-D__USE_MINGW_ANSI_STDIO=1])
        ;;
esac

AC_CONFIG_FILES([
    Makefile
    raqm.pc
    src/Makefile
    src/raqm-version.h
    tests/Makefile
    docs/Makefile
    docs/version.xml
])

AC_OUTPUT
