CFLAGS = -pedantic
CFILES = lstar.c
CC = gcc
BIN_LOC = bin/lstar

$(BIN_LOC): $(CFILES)
	mkdir -p $(@D)
	$(CC) $(CFLAGS) $(CFILES) -o $@

.PHONY: install
install:
	install $(BIN_LOC) /bin/
