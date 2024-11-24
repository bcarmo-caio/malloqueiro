CC=gcc
CFLAGS=\
-Wall \
-Wextra \
-Werror \
-std=gnu11 \
-pedantic \
-no-pie \
-m32 \
-O0

NO_MALLOC_FREE_LIB=no_malloc_free_x86.so
NO_MALLOC_FREE_TEST_X86=test_no_malloc_free_x86

TESTS=\
	test_00 \
	test_01 \
	test_02 \
	test_03 \
	test_04 \
	test_05 \
	test_06 \
	test_07 \
	test_08

all: clean $(NO_MALLOC_FREE_LIB) $(TESTS)

test_%: $(NO_MALLOC_FREE_LIB)
	@$(CC) $(CFLAGS) src/$@.c src/common.c ../lib/malloqueiro.a -o bin/$@

$(NO_MALLOC_FREE_LIB):
	@$(CC) $(CFLAGS) -shared -fPIC src/no_malloc_free_x86.c -o lib/$(NO_MALLOC_FREE_LIB)

run_tests: all
	@t=test_00; ./bin/$$t && echo "$$t: ok"
	@t=test_01; ./bin/$$t && echo "$$t: ok"
	@t=test_02; ./bin/$$t && echo "$$t: ok"
	@t=test_03; ./bin/$$t && echo "$$t: ok"
	@t=test_04; ./bin/$$t && echo "$$t: ok"
	@t=test_05; ./bin/$$t && echo "$$t: ok"
	@t=test_06; ./bin/$$t && echo "$$t: ok"
	@t=test_07; ./bin/$$t && echo "$$t: ok"
	@t=test_08; ./bin/$$t && echo "$$t: ok"

debug: all
	@LD_PRELOAD=lib/$(NO_MALLOC_FREE_LIB) gdb bin/$(TEST_TO_DEBUG)

test_no_malloc_free_x86: all
	@$(CC) $(CFLAGS) src/test_no_malloc_free.c -o bin/test_no_malloc_free_x86
	@LD_PRELOAD=lib/$(NO_MALLOC_FREE_LIB) bin/test_no_malloc_free_x86

clean:
	@rm -rf bin/ lib/
	@mkdir bin lib

.PHONY: all run_tests debug clean
