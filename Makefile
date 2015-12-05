PAKAGES = freetype2 harfbuzz fribidi

CFLAGS = `pkg-config --cflags $(PAKAGES)`
LDLIBS = `pkg-config --libs $(PAKAGES)`

OBJS = test_raqm.o raqm.o

BIN = test_raqm
RUNTEST = runtest
TESTS = $(wildcard tests/*.test)

all: $(BIN)

%.o: %.c raqm.h
	$(CC) -c $< $(CFLAGS) -o $@

$(BIN): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDLIBS)

check: all $(TESTS)
	@bash $(RUNTEST) $(abspath $(BIN)) $(TESTS)

clean:
	rm -f $(BIN) $(OBJS)


