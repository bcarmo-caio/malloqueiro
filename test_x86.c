#include <assert.h>
#include <stdio.h>
#include <stddef.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <inttypes.h>

void *malloca(size_t bytes); /* bytes must be <= 0x0000FFFF*/
void malloca_free(void *ptr);

void new_line(void) {
    int fd;
    char msg[] = "\n";
    fd = open("/dev/stdout", O_WRONLY);
    write(fd, msg, sizeof(msg));
    close(fd);
}

void print_got_from_malloqueiro(void *pointer) {
    /* 32 bit. Note that if this assert fails, it will try to call
     * malloc and the error may be hidden!
     * */
    int fd;
    char got_from_malloca[] = "Got from malloqueiro: 0xzzzzzzzz\n";

    assert(sizeof pointer == 4);
    memset(got_from_malloca, 0, sizeof(got_from_malloca));
    sprintf(got_from_malloca, "Got from malloqueiro: %p\n", pointer);
    fd = open("/dev/stdout", O_WRONLY);
    write(fd, got_from_malloca, sizeof(got_from_malloca));
    close(fd);
}

void _malloca(void **ptr, uint8_t ptr_offset, uint32_t bytes) {
    ptr[ptr_offset] = malloca(bytes);
    print_got_from_malloqueiro(ptr[ptr_offset]);
    new_line();
}

int main(void) {
    void *ptr[3];

    _malloca(ptr, 0, 1);
    malloca_free(ptr[0]);

    /* Uncomment the following line for seg fault =) */
    /* malloca_free(ptr[0]); */

    _malloca(ptr, 1, 5);
    _malloca(ptr, 2, 0xffff);

    /*malloca_free(ptr[0]);*/
    malloca_free(ptr[1]);
    malloca_free(ptr[2]);

    /* Uncomment the following line for "double free or corruption" error */
    malloca_free(ptr[1]);

    /* Uncomment the following line for "too much" error */
    /* _malloca(ptr, 0, 0xffff + 1); */
  return 0;
}
