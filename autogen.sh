#!/bin/sh
# Run this to generate all the initial makefiles, etc.

echo -n "checking for gtkdocize... "
which gtkdocize || {
	echo "*** No gtkdocize (gtk-doc) found, please install it ***"
}

echo -n "checking for autoreconf... "
which autoreconf || {
	echo "*** No autoreconf (autoconf) found, please install it ***"
	exit 1
}

echo "running gtkdocize --copy"
gtkdocize --copy || exit 1

echo "running autoreconf --force --install --verbose"
autoreconf --force --install --verbose || exit $?
