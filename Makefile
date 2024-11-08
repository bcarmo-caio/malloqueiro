CC=gcc
CFLAGS=-Wall -Wextra -W -ansi -pedantic -no-pie -m32
BIN=test

all: clean $(BIN)

$(BIN): test.c malloqueiro-x86.s
	@$(CC) -O0 $(CFLAGS) test.c malloqueiro-x86.s -o $(BIN)

run: clean all
	@./$(BIN)

clean:
	rm -f *.o $(BIN)

.PHONY: all run clean
