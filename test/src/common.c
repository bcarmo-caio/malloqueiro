#include "../include/common.h"

void *malloca(size_t bytes); /* bytes must be <= 0x0000FFFF*/
void malloca_free(void *ptr);

void print_sep(void) {
    if (!DEBUG_ENABLED) { return; }

    int fd;
    char msg[] = "-----------------------------------------------------------------------------------\n\n";
    fd = open("/dev/stdout", O_WRONLY);
    write(fd, msg, sizeof(msg) - 1 /* exclude \0 */);
    close(fd);
}

void _malloca_free(void *ptr) {
    malloca_free(ptr);
    print_sep();
}

void new_line(void) {
    if (!DEBUG_ENABLED) { return; }

    char msg[] = "\n";
    int fd;
    fd = open("/dev/stdout", O_WRONLY);
    write(fd, msg, sizeof(msg) - 1 /* excludes \0*/);
    close(fd);
}

void print_got_from_malloqueiro(void *ptr) {
    if (!DEBUG_ENABLED) { return; }

    /* 32 bit. Note that if this assert fails, it will try to call
     * malloc and the error may be hidden!
     * */
    char msg[40];
    uint8_t msg_len;
    int fd;
    /* if msg was 61, we could get aligned to 64 bytes */

    assert(sizeof ptr == 4);
    msg_len = sprintf(msg, "Got from malloqueiro: %p\n",  ptr);
    fd = open("/dev/stdout", O_WRONLY);
    write(fd, msg, msg_len);
    close(fd);
}

void _malloca(void **ptr, uint8_t ptr_offset, uint32_t bytes) {
    ptr[ptr_offset] = malloca(bytes);
    print_got_from_malloqueiro(ptr[ptr_offset]);
    new_line();
    print_sep();
}

void *syscall_current_brk(void) {
    return sbrk(0);
}
