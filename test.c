#include <assert.h>
#include <stdio.h>
#include <stddef.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

void *malloca(size_t bytes); /* bytes must be <= 0x0000FFFF*/

#define new_line() { \
    char msg[] = "\n"; \
    fd = open("/dev/stdout", O_WRONLY); \
    write(fd, msg, sizeof(msg)); \
    close(fd); \
}

#define print_got_from_malloqueiro(pointer) { \
    /* 32 bit. Note that if this assert fails, it will try to call
     * malloc and the error may be hidden!
     * */ \
    assert(sizeof pointer == 4); \
    memset(got_from_malloca, 0, sizeof(got_from_malloca)); \
    sprintf(got_from_malloca, "Got from malloqueiro: %p\n", pointer); \
    fd = open("/dev/stdout", O_WRONLY); \
    write(fd, got_from_malloca, sizeof(got_from_malloca));            \
    close(fd); \
}

int main() {
    int fd;
    char got_from_malloca[] = "Got from malloqueiro: 0xzzzzzzzz\n";
    void *ptr;
    ptr = malloca(1);
    print_got_from_malloqueiro(ptr); new_line();
    ((char*) ptr)[0] = 'a';
    print_got_from_malloqueiro(malloca(5));          new_line();
    print_got_from_malloqueiro(malloca(0xffff));     new_line();
    print_got_from_malloqueiro(malloca(0xffff + 1)); new_line();
  return 0;
}