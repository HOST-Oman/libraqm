NULL = 

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = src tests

MAINTAINERCLEANFILES = \
    $(GITIGNORE_MAINTAINERCLEANFILES_TOPLEVEL) \
    $(GITIGNORE_MAINTAINERCLEANFILES_M4_LIBTOOL) \
    $(GITIGNORE_MAINTAINERCLEANFILES_MAKEFILE_IN) \
    $(NULL)

-include $(top_srcdir)/git.mk
pkgconfiglibdir = $(libdir)/pkgconfig
pkgconfiglib_DATA = raqm.pc
