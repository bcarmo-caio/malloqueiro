#include "../include/common.h"

int main(void) {
    void *ptr[2];
    const void *initial_brk = syscall_current_brk();
    void *cur_brk = NULL;
    void *head = NULL;
    void *tail = NULL;
    chunk_t *chunk = NULL;

    assert(sizeof(chunk_t) == 8); /* validate our __packed__ is being respected lol */

    /* Test case
     * Allocate 2 chunks and free the tail of the list.
     * We should end up with just one chunk. */

    _malloca(ptr, 0, 1); /* [ U */
    _malloca(ptr, 1, 2); /* [ U, U ] */

    memset(ptr[0], 'P', 1);
    /* Up to here is already tested in previous test cases*/

    /* ---------------------------------------------------------------------- */

    _malloca_free(ptr[1]); /* [ U ] */

    /* Initial brk should always have the same value */
    assert(get_initial_brk() == initial_brk);

    /* Validate head pointer */
    head = get_head();
    assert(head == initial_brk);

    /* Validate chunk metadata integrity. */
    chunk = head;
    assert(chunk->used == (uint8_t) 1);
    assert(chunk->next == NULL);
    assert(chunk->size == (uint16_t) 1);

    /* Validate chunk user data integrity. */
    assert(*((char *) ((uint8_t *) chunk + METADATA_SIZE)) == 'P');

    /* Validate tail pointer. Should be head now */
    tail = get_tail();
    assert(head == tail);

    /* Validate current brk */
    cur_brk = get_current_brk();
    assert(cur_brk == (uint8_t *) initial_brk + METADATA_SIZE + 1);

    /* ---------------------------------------------------------------------- */

    /* Freeing memory before leaving test case
     * Already tested case when [ U ] -> [ - ] */
    print_sep();
    _malloca_free(ptr[0]); /* [ - ] */
    print_sep();

    assert(get_head() == NULL);
    assert(get_tail() == NULL);
    assert(get_current_brk() == (uint8_t *) ptr[0] - METADATA_SIZE);

    return 0;
}
