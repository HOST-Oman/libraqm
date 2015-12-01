CFLAGS = `pkg-config --cflags freetype2 harfbuzz fribidi`
LDLIBS = `pkg-config --libs freetype2 harfbuzz fribidi`

all: test_raqm

raqm.o: raqm.c raqm.h
	gcc -c raqm.c $(CFLAGS) -o $@

test_raqm.o: test_raqm.c raqm.h
	gcc -c test_raqm.c $(CFLAGS) -o $@

test_raqm: test_raqm.o raqm.o
	gcc test_raqm.o raqm.o -o $@ $(LDLIBS)




