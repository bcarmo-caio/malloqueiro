#include "../include/common.h"

void *malloca(size_t bytes); /* bytes must be <= 0x0000FFFF*/
void malloca_free(void *ptr);

void print_sep(void) {
    char msg[] = "-----------------------------------------------------------------------------------\n\n";
    int fd;

    if (!DEBUG_ENABLED) { return; }

    fd = open("/dev/stdout", O_WRONLY);
    write(fd, msg, sizeof(msg) - 1 /* exclude \0 */);
    close(fd);
}

void _malloca_free(void *ptr) {
    malloca_free(ptr);
    print_sep();
}

void new_line(void) {
    char msg[] = "\n";
    int fd;

    if (!DEBUG_ENABLED) { return; }

    fd = open("/dev/stdout", O_WRONLY);
    write(fd, msg, sizeof(msg) - 1 /* excludes \0*/);
    close(fd);
}

void print_got_from_malloqueiro(void *ptr) {
    /* 32 bit. Note that if this assert fails, it will try to call
     * malloc and the error may be hidden!
     * */
    char msg[40];
    uint8_t msg_len;
    int fd;
    /* if msg was 61, we could get aligned to 64 bytes */

    if (!DEBUG_ENABLED) { return; }

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

__attribute__((naked))
void *syscall_current_brk(void) {
    /* sbrk is not in ANSI C and I don't want to read
       from "/proc/self/maps" file. */
    __asm__ (
        "push %ebx \n\t"
        "movl $45, %eax\n\t"
        "movl $0, %ebx\n\t"
        "int $0x80\n\t"
        "pop %ebx \n\t"
        "ret\n\t"
    );
}
