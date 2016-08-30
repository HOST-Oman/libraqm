NULL = 

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = src docs tests

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = @PACKAGE@.pc

MAINTAINERCLEANFILES = \
    $(GITIGNORE_MAINTAINERCLEANFILES_TOPLEVEL) \
    $(GITIGNORE_MAINTAINERCLEANFILES_M4_LIBTOOL) \
    $(GITIGNORE_MAINTAINERCLEANFILES_MAKEFILE_IN) \
    $(srcdir)/gtk-doc.make \
    $(srcdir)/m4/gtk-doc.m4 \
    $(srcdir)/docs/raqm-overrides.txt \
    $(NULL)

-include $(top_srcdir)/git.mk
