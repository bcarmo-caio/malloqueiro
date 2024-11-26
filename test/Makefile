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
	test_08 \
	test_09 \
	test_10 \
	test_11 \
	test_12

all: $(NO_MALLOC_FREE_LIB) $(TESTS)

test_%: $(NO_MALLOC_FREE_LIB)
	@$(CC) $(CFLAGS) src/$@.c src/common.c ../lib/malloqueiro.a -o bin/$@

$(NO_MALLOC_FREE_LIB):
	@$(CC) $(CFLAGS) -shared -fPIC src/no_malloc_free_x86.c -o lib/$(NO_MALLOC_FREE_LIB)

run_tests:
	@t=test_00; LD_PRELOAD=lib/$(NO_MALLOC_FREE_LIB) ./bin/$$t && echo "$$t: ok"
	@t=test_01; LD_PRELOAD=lib/$(NO_MALLOC_FREE_LIB) ./bin/$$t && echo "$$t: ok"
	@t=test_02; LD_PRELOAD=lib/$(NO_MALLOC_FREE_LIB) ./bin/$$t && echo "$$t: ok"
	@t=test_03; LD_PRELOAD=lib/$(NO_MALLOC_FREE_LIB) ./bin/$$t && echo "$$t: ok"
	@t=test_04; LD_PRELOAD=lib/$(NO_MALLOC_FREE_LIB) ./bin/$$t && echo "$$t: ok"
	@t=test_05; LD_PRELOAD=lib/$(NO_MALLOC_FREE_LIB) ./bin/$$t && echo "$$t: ok"
	@t=test_06; LD_PRELOAD=lib/$(NO_MALLOC_FREE_LIB) ./bin/$$t && echo "$$t: ok"
	@t=test_07; LD_PRELOAD=lib/$(NO_MALLOC_FREE_LIB) ./bin/$$t && echo "$$t: ok"
	@t=test_08; LD_PRELOAD=lib/$(NO_MALLOC_FREE_LIB) ./bin/$$t && echo "$$t: ok"
	@t=test_09; LD_PRELOAD=lib/$(NO_MALLOC_FREE_LIB) ./bin/$$t && echo "$$t: ok"
	@t=test_10; LD_PRELOAD=lib/$(NO_MALLOC_FREE_LIB) ./bin/$$t && echo "$$t: ok"
	@t=test_11; LD_PRELOAD=lib/$(NO_MALLOC_FREE_LIB) ./bin/$$t && echo "$$t: ok"
	@t=test_12; LD_PRELOAD=lib/$(NO_MALLOC_FREE_LIB) ./bin/$$t && echo "$$t: ok"

run_single_test:
	@LD_PRELOAD=lib/$(NO_MALLOC_FREE_LIB) ./bin/$(TEST) && echo "$(TEST): ok"

debug:
	@LD_PRELOAD=lib/$(NO_MALLOC_FREE_LIB) gdb bin/$(TEST)

test_no_malloc_free_x86:
	@$(CC) $(CFLAGS) src/test_no_malloc_free.c -o bin/test_no_malloc_free_x86
	@LD_PRELOAD=lib/$(NO_MALLOC_FREE_LIB) bin/test_no_malloc_free_x86

clean:
	@rm -rf bin/ lib/
	@mkdir bin lib

.PHONY: all run_tests debug clean
