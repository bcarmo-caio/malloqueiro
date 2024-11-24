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
AR=ar
ARFLAGS=\
	rcs \
	--target=elf32-i386

all: clean lib/malloqueiro.a

lib/%.a: obj/%.o
	@$(AR) $(ARFLAGS) $@ $<

obj/%.o: src/%.s
	@$(CC) $(CFLAGS) -fPIC -c $< -o $@

clean:
	@rm -rf obj lib
	@mkdir obj lib

.PHONY: all clean
