CC=gcc
CFLAGS=\
	-Wall \
	-Wextra \
	-Werror \
	-ansi \
	-pedantic \
	-no-pie \
	-m32 \
	-O0
BIN=test_x86
NO_MALLOC_FREE_LIB=no_malloc_free_x86.so
NO_MALLOC_FREE_TEST_X86=test_no_malloc_free_x86

all: clean $(BIN) $(NO_MALLOC_FREE_LIB)

$(BIN): $(NO_MALLOC_FREE_LIB) test_x86.c malloqueiro-x86.s
	@$(CC) $(CFLAGS) test_x86.c malloqueiro-x86.s -o $(BIN)

$(NO_MALLOC_FREE_LIB): no_malloc_free_x86.c
	@$(CC) $(CFLAGS) -shared -fPIC -o $(NO_MALLOC_FREE_LIB) no_malloc_free_x86.c

run: all
	@LD_PRELOAD=./$(NO_MALLOC_FREE_LIB) ./$(BIN)

debug: all
	@LD_PRELOAD=./$(NO_MALLOC_FREE_LIB) gdb ./$(BIN)

test_no_malloc_free_x86: all test_no_malloc_free.c
	@$(CC) $(CFLAGS) test_no_malloc_free.c -o test_no_malloc_free_x86
	@LD_PRELOAD=./$(NO_MALLOC_FREE_LIB) ./test_no_malloc_free_x86

clean:
	@rm -f *.o $(BIN) $(NO_MALLOC_FREE_LIB) test_no_malloc_free_x86

.PHONY: all run debug clean
