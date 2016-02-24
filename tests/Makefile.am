
check_PROGRAMS =
TESTS =

include Makefile.sources

raqm_test_SOURCES = $(RAQM_TEST_sources)

raqm_test_CPPFLAGS = \
    -I$(top_srcdir)/src/ \
    -I$(top_builddir)/src/ \
    $(GLIB_CFLAGS) \
    $(FREETYPE_CFLAGS) \
    $(HARFBUZZ_CFLAGS) \
    $(FRIBIDI_CFLAGS) \
    $(WARN_CFLAGS) \
    $(NULL)

raqm_test_LDADD = \
    $(top_builddir)/src/libraqm-test.la \
    $(GLIB_LIBS) \
    $(FREETYPE_LIBS) \
    $(HARFBUZZ_LIBS) \
    $(FRIBIDI_LIBS) \
    $(WARN_LDLAGS)
    $(NULL)

TEST_FILES = $(RAQM_TEST_files)
if HAVE_GLIB
check_PROGRAMS += raqm-test
TESTS += $(TEST_FILES)
endif # HAVE_GLIB

TEST_EXTENSIONS = \
    .test \
    $(NULL)

AM_TESTS_ENVIRONMENT = \
    EXEEXT="$(EXEEXT)"; \
    export EXEEXT; \
    srcdir="$(srcdir)"; \
    export srcdir; \
    builddir="$(builddir)"; \
    export builddir; \
    $(NULL)

TEST_LOG_COMPILER = sh $(srcdir)/runtest

EXTRA_DIST = \
    $(TEST_FILES) \
    fonts/sha1sum \
    runtest \
    $(NULL)

update-tests:
	@if ! $(AM_V_P); then echo "  GEN      test references"; fi
	$(AM_V_at)$(AM_TESTS_ENVIRONMENT) \
	          $(TEST_LOG_COMPILER) --reference $(TEST_FILES)

-include $(top_srcdir)/git.mk
