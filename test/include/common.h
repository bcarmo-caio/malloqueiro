#ifndef MALLOQUEIRO_TEST_X86_COMMON_H
#define MALLOQUEIRO_TEST_X86_COMMON_H

#include <assert.h>
#include <stdio.h>
#include <stddef.h>
#include <unistd.h>
#include <fcntl.h>
#include <inttypes.h>
#include <sys/syscall.h>
#include <string.h>
#include <stdbool.h>

#define DEBUG_ENABLED false

/*
 * In all the following test cases,
 * U means the chunk is used
 * F means the chunk is free
 * so the list [ F, U, U, F, U ] means we have 5 chunks being
 * the first Free
 * the second Used
 * the third Used
 * the forth Free
 * the fifth Used
 * and so on.
 *
 * [ - ] notation means that brk should be set to initial
 *       brk value, head and tail should be 0 (null)
 */

/* START Function exported by malloqueiro.s */
__attribute_maybe_unused__ void breakpoint(void);
void *get_initial_brk(void);
void *get_head(void);
void *get_tail(void);
void *get_current_brk(void);
/* END Function exported by malloqueiro.s */

typedef struct {
    uint8_t used; /* used is 1 byte and is located at the beginning */
    void *next;   /* Right after 'used' we have a 4 bytes variable: next */
    /* size stores the user requested bytes, not the total chunk size, which is size + metadata size*/
    uint16_t size; /* Right after 'next' we have a 3 bytes (but only 2 used) variable: size */
    __attribute__((unused)) uint8_t padding; /* never used */
} __attribute__((packed)) chunk_t;

#define METADATA_SIZE sizeof(chunk_t)

void *syscall_current_brk(void);
void print_sep(void);
void _malloca_free(void *ptr);
void new_line(void);
void print_got_from_malloqueiro(void *ptr);
void _malloca(void **ptr, uint8_t ptr_offset, uint32_t bytes);

#endif /* MALLOQUEIRO_TEST_X86_COMMON_H */
