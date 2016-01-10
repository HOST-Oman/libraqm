NULL = 

check_PROGRAMS =
TESTS =

raqm_test_SOURCES = \
    raqm-test.c \
    $(NULL)

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

TEST_FILES = \
    features-arabic.test \
    features-kerning.test \
    features-ligature.test \
    scripts-backward.test \
    scripts-backward-ltr.test \
    scripts-backward-rtl.test \
    scripts-forward.test \
    scripts-forward-ltr.test \
    scripts-forward-rtl.test \
    xyoffset.test \
    test1.test \
    test2.test \
    test3.test \
    test4.test \
    test5.test \
    test1_LTR.test \
    test2_LTR.test \
    test3_LTR.test \
    test4_LTR.test \
    test5_LTR.test \
    test1_RTL.test \
    test2_RTL.test \
    test3_RTL.test \
    test4_RTL.test \
    test5_RTL.test \
    $(NULL)

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

-include $(top_srcdir)/git.mk
