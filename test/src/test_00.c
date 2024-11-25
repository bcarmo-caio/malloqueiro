#include "../include/common.h"

int main(void) {
    void *ptr[1];
    const void *initial_brk = syscall_current_brk();
    void *cur_brk = NULL;
    void *head = NULL;
    void *tail = NULL;
    chunk_t *chunk = NULL;

    assert(sizeof(chunk_t) == 8); /* validate our __packed__ is being respected lol */
    print_sep();

    /* Test case:
     * Allocate 1 chunk and then free it. This should reset brk */

    /* ---------------------------------------------------------------------- */

    _malloca(ptr, 0, 1); /* [ U ] */
    memset(ptr[0], 'c', 1);

    head = get_head();
    tail = get_tail();
    chunk = head;
    cur_brk = get_current_brk();

    assert(get_initial_brk() == initial_brk);
    assert(head == initial_brk);
    assert(tail == initial_brk);
    assert(chunk->used == (uint8_t) 1);
    assert(chunk->next == NULL);
    assert(chunk->size == (uint16_t) 1);
    assert(cur_brk == (void *) ((uint8_t *) initial_brk + 1 + METADATA_SIZE));
    /* Check if malloqueiro returns the right memory region
     * and if we can set user space region properly */
    assert(ptr[0] == (void *) ((uint8_t *) initial_brk + METADATA_SIZE));
    assert(*((char *) ((uint8_t *) initial_brk + METADATA_SIZE)) == 'c');

    /* ---------------------------------------------------------------------- */

    print_sep();
    _malloca_free(ptr[0]); /* [ - ] */
    print_sep();

    /* Validate everything got reset */
    assert(get_initial_brk() == initial_brk);
    assert(get_current_brk() == initial_brk);
    assert(get_head() == NULL);
    assert(get_tail() == NULL);

    return 0;
}
