CC=gcc
CFLAGS=-Wall -Wextra -W -ansi -pedantic -no-pie -m32 -O0
BIN=test
NO_MALLOC_LIB=no_malloc_32.so

all: clean $(BIN) $(NO_MALLOC_LIB)

$(BIN): $(NO_MALLOC_LIB) test.c malloqueiro-x86.s
	@$(CC) $(CFLAGS) test.c malloqueiro-x86.s -o $(BIN)

$(NO_MALLOC_LIB): no_malloc_32.c
	@$(CC) $(CFLAGS) -shared -fPIC -o $(NO_MALLOC_LIB) no_malloc_32.c

run: all
	LD_PRELOAD=./$(NO_MALLOC_LIB) ./$(BIN)

clean:
	rm -f *.o $(BIN) $(NO_MALLOC_LIB)

.PHONY: all run clean
