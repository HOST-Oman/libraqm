PAKAGES = freetype2 harfbuzz fribidi glib-2.0

WARNINGS = -W -Wall -Wextra -Wformat=2 -Wstrict-prototypes \
           -Wimplicit-function-declaration -Wredundant-decls \
           -Wdeclaration-after-statement -Wconversion \
           -Wsign-conversion -Winit-self -Wundef -Wshadow \
           -Wpointer-arith -Wreturn-type -Wsign-compare \
           -Wmultichar -Wformat-nonliteral -Wuninitialized \
           -Wformat-security -pedantic \
           -Wno-variadic-macros \
	   -Werror

EXTRA = $(shell pkg-config --atleast-version=0.19.8 fribidi && echo "-DHAVE_FRIBIDI_REORDER_RUNS")

CFLAGS = $(shell pkg-config --cflags $(PAKAGES) | sed -e "s/-I/-isystem/g") $(WARNINGS) $(EXTRA) -DRAQM_TESTING -g
LDLIBS = $(shell pkg-config --libs $(PAKAGES))

SRC = src
TEST = tests
OBJS = $(TEST)/raqm-test.o $(SRC)/raqm.o

BIN = $(TEST)/raqm-test
RUNTEST = tests/runtest
UPDATETEST = tests/updatetest
TESTS = $(wildcard $(TEST)/*.test)

all: $(BIN)

%.o: %.c $(SRC)/raqm.h
	$(CC) -c $< -I$(SRC) $(CFLAGS) -o $@

$(BIN): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDLIBS)

check: all $(TESTS)
	@bash $(RUNTEST) $(abspath $(BIN)) "$(TESTS)"

update-tests: all $(TESTS)
	@bash $(UPDATETEST) $(abspath $(BIN)) "$(TESTS)"

clean:
	rm -f $(BIN) $(OBJS)


