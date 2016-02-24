
lib_LTLIBRARIES = libraqm.la
check_LTLIBRARIES = libraqm-test.la

include Makefile.source

libraqm_la_SOURCES = $(RAQM_BASE_sources)
include_HEADERS = $(RAQM_INCLUDE_headers)

libraqm_la_LIBADD = \
    $(FREETYPE_LIBS) \
    $(HARFBUZZ_LIBS) \
    $(FRIBIDI_LIBS) \
    $(WARN_LDLAGS)
    $(NULL)

libraqm_la_CPPFLAGS = \
    $(FREETYPE_CFLAGS) \
    $(HARFBUZZ_CFLAGS) \
    $(FRIBIDI_CFLAGS) \
    $(WARN_CFLAGS) \
    $(NULL)

libraqm_test_la_SOURCES = $(libraqm_la_SOURCES)
libraqm_test_la_LIBADD = $(libraqm_la_LIBADD)
libraqm_test_la_CPPFLAGS = $(libraqm_la_CPPFLAGS) -DRAQM_TESTING

-include $(top_srcdir)/git.mk
