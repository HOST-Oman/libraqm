PAKAGES = freetype2 harfbuzz fribidi

CFLAGS = `pkg-config --cflags $(PAKAGES)`
LDLIBS = `pkg-config --libs $(PAKAGES)`

OBJS = test_raqm.o raqm.o

BIN = test_raqm

all: $(BIN)

%.o: %.c raqm.h
	$(CC) -c $< $(CFLAGS) -o $@

$(BIN): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDLIBS)

clean:
	rm -f $(BIN) $(OBJS)


