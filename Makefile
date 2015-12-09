PAKAGES = freetype2 harfbuzz fribidi

CFLAGS = `pkg-config --cflags $(PAKAGES)`
LDLIBS = `pkg-config --libs $(PAKAGES)`

OBJS = raqm-test.o raqm.o

BIN = raqm-test
RUNTEST = tests/runtest
TESTS = $(wildcard tests/*.test)

all: $(BIN)

%.o: %.c raqm.h
	$(CC) -c $< $(CFLAGS) -o $@

$(BIN): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDLIBS)

check: all $(TESTS)
	@bash $(RUNTEST) $(abspath $(BIN)) "$(TESTS)"

clean:
	rm -f $(BIN) $(OBJS)


