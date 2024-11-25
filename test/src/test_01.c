#include "../include/common.h"

int main(void) {
    void *ptr[2];
    const void *initial_brk = syscall_current_brk();
    void *cur_brk = NULL;
    void *head = NULL;
    void *tail = NULL;
    chunk_t *chunk = NULL;

    assert(sizeof(chunk_t) == 8); /* validate our __packed__ is being respected lol */
    print_sep();

    /* Test case:
     * Allocate 2 chunks, free the first and then the second one.
     * When freeing the second pointer (first chunk) brk should get reset
     * */

    /* ---------------------------------------------------------------------- */

    _malloca(ptr, 0, 1); /* [ U ]    */
    _malloca(ptr, 1, 2); /* [ U, U ] */
    memset(ptr[0], 'c', 1);
    memset(ptr[1], 'A', 1);
    memset((uint8_t *) ptr[1] + 1, 'B', 1);

    head = get_head();
    tail = get_tail();
    cur_brk = get_current_brk();
    chunk = head;

    /* Initial brk should always have the same value */
    assert(get_initial_brk() == initial_brk);

    /* We validate in test 1 with just one chunk, validate with 2 chunks */
    assert(head == initial_brk);

    assert(chunk->used == (uint8_t) 1);
    assert(chunk->next == tail);
    assert(chunk->size == (uint16_t) 1);

    /* Check if malloqueiro returns the right memory region
     * and if we can set user space region properly for the second pointer */
    assert(ptr[0] == (void *) ((uint8_t *) initial_brk + METADATA_SIZE));
    assert(*((char *) ((uint8_t *) initial_brk + METADATA_SIZE)) == 'c');

    /* Validate tail. Should be at the second chunk */
    assert(tail == (void *) ((char *) initial_brk +
                                      + 1 + METADATA_SIZE));

    /* Check if malloqueiro returns the right memory region for the first pointer
     * and if we can set user space region properly */
    assert(ptr[1] == (void *) ((uint8_t *) tail + METADATA_SIZE));
    assert(*(char *)( (uint8_t *) ptr[1]) == 'A');
    assert(*(char *)( (uint8_t *) ptr[1] + 1) == 'B');

    /* Validate tail chunk metadata */
    chunk = tail;
    assert(chunk->used == (uint8_t) 1);
    assert(chunk->next == NULL);
    assert(chunk->size == (uint16_t) 2);

    /* Validate current brk is properly set */
    assert(cur_brk == (void *) ((char *) initial_brk +
                                + 1 + METADATA_SIZE +
                                + 2 + METADATA_SIZE));

    /* ---------------------------------------------------------------------- */

    _malloca_free(ptr[0]); /* [ F, U ] */

    /* Initial brk should always have the same value */
    assert(get_initial_brk() == initial_brk);

    /* Validate head pointer and chunk metadata. Should be free now */
    assert(head == initial_brk);

    chunk = head;
    assert(chunk->used == (uint8_t) 0);
    assert(chunk->next == tail);
    assert(chunk->size == (uint16_t) 1);

    /* Validate tail pointer and chunk metadata. Should be the same */
    assert(tail == (void *) ((char *) initial_brk +
                                      + 1 + METADATA_SIZE));
    chunk = tail;
    assert(chunk->used == (uint8_t) 1);
    assert(chunk->next == NULL);
    assert(chunk->size == (uint16_t) 2);

    /* ---------------------------------------------------------------------- */

    /* Freeing memory before leaving test case */
    print_sep();
    _malloca_free(ptr[1]); /* [ - ] */
    print_sep();

    /* Initial brk should always have the same value */
    assert(get_initial_brk() == initial_brk);

    /* Validate everything got reset */
    assert(get_initial_brk() == initial_brk);
    assert(get_current_brk() == initial_brk);
    assert(get_head() == NULL);
    assert(get_tail() == NULL);

    return 0;
}
