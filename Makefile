PAKAGES = freetype2 harfbuzz fribidi

WARNINGS = -W -Wall -Wextra -Wformat=2 -Wstrict-prototypes \
           -Wimplicit-function-declaration -Wredundant-decls \
           -Wdeclaration-after-statement -Wconversion \
           -Wsign-conversion -Winit-self -Wundef -Wshadow \
           -Wpointer-arith -Wreturn-type -Wsign-compare \
           -Wmultichar -Wformat-nonliteral -Wuninitialized \
           -Wformat-security -pedantic

CFLAGS = $(shell pkg-config --cflags $(PAKAGES) | sed -e "s/-I/-isystem/g") $(WARNINGS) -DTESTING
LDLIBS = $(shell pkg-config --libs $(PAKAGES))

OBJS = raqm-test.o raqm.o

BIN = raqm-test
RUNTEST = tests/runtest
UPDATETEST = tests/updatetest
TESTS = $(wildcard tests/*.test)

all: $(BIN)

%.o: %.c raqm.h
	$(CC) -c $< $(CFLAGS) -o $@

$(BIN): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDLIBS)

check: all $(TESTS)
	@bash $(RUNTEST) $(abspath $(BIN)) "$(TESTS)"

update-tests: all $(TESTS)
	@bash $(UPDATETEST) $(abspath $(BIN)) "$(TESTS)"

clean:
	rm -f $(BIN) $(OBJS)


