NULL = 

lib_LTLIBRARIES = libraqm.la
check_LTLIBRARIES = libraqm-test.la

libraqm_la_SOURCES = \
    raqm.c \
    raqm.h \
    raqm-version.h \
    $(NULL)

include_HEADERS = \
    raqm.h \
    raqm-version.h \
    $(NULL)

libraqm_la_LIBADD = \
    $(FREETYPE_LIBS) \
    $(HARFBUZZ_LIBS) \
    $(FRIBIDI_LIBS) \
    $(WARN_LDLAGS) \
    $(NULL)

libraqm_la_CPPFLAGS = \
    $(FREETYPE_CFLAGS) \
    $(HARFBUZZ_CFLAGS) \
    $(FRIBIDI_CFLAGS) \
    $(WARN_CFLAGS) \
    $(NULL)

libraqm_la_LDFLAGS = \
    -version-info $(RAQM_LIBTOOL_VERSION) \
    $(NULL)

libraqm_test_la_SOURCES = $(libraqm_la_SOURCES)
libraqm_test_la_LIBADD = $(libraqm_la_LIBADD)
libraqm_test_la_CPPFLAGS = $(libraqm_la_CPPFLAGS) -DRAQM_TESTING

-include $(top_srcdir)/git.mk
