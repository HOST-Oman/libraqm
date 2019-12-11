NULL = 

check_PROGRAMS =
TESTS =

raqm_test_SOURCES = \
    raqm-test.c \
    $(NULL)

raqm_test_CPPFLAGS = \
    -I$(top_srcdir)/src/ \
    -I$(top_builddir)/src/ \
    $(FREETYPE_CFLAGS) \
    $(HARFBUZZ_CFLAGS) \
    $(FRIBIDI_CFLAGS) \
    $(WARN_CFLAGS) \
    $(NULL)

raqm_test_LDADD = \
    $(top_builddir)/src/libraqm-test.la \
    $(FREETYPE_LIBS) \
    $(HARFBUZZ_LIBS) \
    $(FRIBIDI_LIBS) \
    $(WARN_LDLAGS)
    $(NULL)

TEST_FILES = \
    buffer-flags-1.test \
    direction-ltr-1.test \
    direction-ltr-2.test \
    direction-ltr-3.test \
    direction-ltr-4.test \
    direction-ltr-5.test \
    direction-rtl-1.test \
    direction-rtl-2.test \
    direction-rtl-3.test \
    direction-rtl-4.test \
    direction-rtl-5.test \
    direction-ttb-1.test \
    empty-text.test \
    features-arabic.test \
    features-kerning.test \
    features-ligature.test \
    scripts-backward.test \
    scripts-backward-ltr.test \
    scripts-backward-rtl.test \
    scripts-common.test \
    scripts-forward.test \
    scripts-forward-ltr.test \
    scripts-forward-rtl.test \
    languages-sr.test \
    languages-sr-ru.test \
    multi-fonts.test \
    multi-fonts2.test \
    xyoffset.test \
    test1.test \
    test2.test \
    test3.test \
    test4.test \
    test5.test \
    cursor_position1.test \
    cursor_position2.test \
    cursor_position3.test \
    cursor_position4.test \
    cursor_position_GB3.test \
    cursor_position_GB4.test \
    cursor_position_GB5.test \
    cursor_position_GB8a.test \
    cursor_position_GB9.test \
    cursor_position_GB9a.test \
    invisible_glyph_hidden.test \
    invisible_glyph_space.test \
    invisible_glyph_explicit.test
    $(NULL)


check_PROGRAMS += raqm-test
TESTS += $(TEST_FILES)

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

TEST_LOG_COMPILER = python3 $(srcdir)/runtest.py

EXTRA_DIST = \
    $(TEST_FILES) \
    fonts/sha1sum \
    runtest.py \
    $(NULL)

update-tests:
	@if ! $(AM_V_P); then echo "  GEN      test references"; fi
	$(AM_V_at)$(AM_TESTS_ENVIRONMENT) \
	          $(TEST_LOG_COMPILER) --reference $(TEST_FILES)

-include $(top_srcdir)/git.mk
