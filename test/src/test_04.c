#include "../include/common.h"

int main(void) {
    void *ptr[4];
    const void *initial_brk = syscall_current_brk();
    void *cur_brk = NULL;
    void *head = NULL;
    void *tail = NULL;
    chunk_t *chunk = NULL;

    uint16_t offset = 0;

    assert(sizeof(chunk_t) == 8); /* validate our __packed__ is being respected lol */
    print_sep();

    /* Test case:
     * Merge with previous and previous is not head.
     * Free userspace size should be:
     * what user asked for previous pointer +
     *           (what asked for current pointer + metadata)
     */

    _malloca(ptr, 0, 1);  /* [ U ] */
    _malloca(ptr, 1, 2);  /* [ U, U ] */
    _malloca(ptr, 2, 3);  /* [ U, U, U ] */
    _malloca(ptr, 3, 4);  /* [ U, U, U, U ] */

    /* Initial brk should always have the same value */
    assert(get_initial_brk() == initial_brk);

    memset((uint8_t *) ptr[0] + 0, '1', 1);

    memset((uint8_t *) ptr[1] + 0, 'a', 1);
    memset((uint8_t *) ptr[1] + 1, 'b', 1);

    memset((uint8_t *) ptr[2] + 0, 'A', 1);
    memset((uint8_t *) ptr[2] + 1, 'B', 1);
    memset((uint8_t *) ptr[2] + 2, 'C', 1);

    memset((uint8_t *) ptr[3] + 0, 'w', 1);
    memset((uint8_t *) ptr[3] + 1, 'x', 1);
    memset((uint8_t *) ptr[3] + 2, 'y', 1);
    memset((uint8_t *) ptr[3] + 3, 'z', 1);

    /* Check if malloqueiro returns the right memory region
     * and if we can set user space region properly for all pointers */
    offset = METADATA_SIZE;
    assert(ptr[0] == (void *) ((uint8_t *) initial_brk + offset));
    assert(*((char *) ((uint8_t *) initial_brk + offset)) == '1');

    /*        1 byte for the first chunk */
    offset = (1 + METADATA_SIZE) + METADATA_SIZE;
    assert(ptr[1] == (void *) ((uint8_t *) initial_brk + offset));
    assert(*((char *) ((uint8_t *) initial_brk + offset++)) == 'a');
    assert(*((char *) ((uint8_t *) initial_brk + offset)) == 'b');

    /*        1 byte for the first, 2 for the second chunk */
    offset = (1 + METADATA_SIZE) + (2 + METADATA_SIZE) + METADATA_SIZE;
    assert(ptr[2] == (void *) ((uint8_t *) initial_brk + offset));
    assert(*((char *) ((uint8_t *) initial_brk + offset++)) == 'A');
    assert(*((char *) ((uint8_t *) initial_brk + offset++)) == 'B');
    assert(*((char *) ((uint8_t *) initial_brk + offset)) == 'C');

    /*        1 byte for the first, 2 for the second and  3 for the third chunk */
    offset = (1 + METADATA_SIZE) + (2 + METADATA_SIZE) + (3 + METADATA_SIZE) + METADATA_SIZE;
    assert(ptr[3] == (void *) ((uint8_t *) initial_brk + offset));
    assert(*((char *) ((uint8_t *) initial_brk + offset++)) == 'w');
    assert(*((char *) ((uint8_t *) initial_brk + offset++)) == 'x');
    assert(*((char *) ((uint8_t *) initial_brk + offset++)) == 'y');
    assert(*((char *) ((uint8_t *) initial_brk + offset)) == 'z');

    /* Validate head pointer */
    head = get_head();
    assert(head == initial_brk);

    /* Validate tail pointer */
    tail = get_tail();
    assert(tail == (uint8_t *) ptr[3] - METADATA_SIZE);

    /* Validate brk */
    cur_brk = get_current_brk();
    /* User requested 4 bytes for the last pointer */
    assert(cur_brk == (uint8_t *) ptr[3] + 4);

    /* Validate first chunk */
    chunk = head;
    assert(chunk->used == (uint8_t) 1);
    assert(chunk->next == (uint8_t *) ptr[1] - METADATA_SIZE);
    assert(chunk->size == (uint16_t) 1);

    /* Validate second chunk */
    chunk = chunk->next;
    assert(chunk->used == (uint8_t) 1);
    assert(chunk->next == (uint8_t *) ptr[2] - METADATA_SIZE);
    assert(chunk->size == (uint16_t) 2);

    /* Validate third chunk */
    chunk = chunk->next;
    assert(chunk->used == (uint8_t) 1);
    assert(chunk->next == (uint8_t *) ptr[3] - METADATA_SIZE);
    assert(chunk->size == (uint16_t) 3);

    /* Validate fourth chunk */
    chunk = chunk->next;
    assert(chunk->used == (uint8_t) 1);
    assert(chunk->next == NULL);
    assert(chunk->size == (uint16_t) 4);

    /* ---------------------------------------------------------------------- */

    _malloca_free(ptr[2]); /* [ U, F, U, U ] */
    _malloca_free(ptr[1]); /* [ U, F, U, ] */

    /* Initial brk should always have the same value */
    assert(get_initial_brk() == initial_brk);

    /* Validate head and data integrity */
    chunk = head;
    assert(head == initial_brk);
    assert(chunk->used == (uint8_t) 1);
    assert(chunk->next == (uint8_t *) ptr[1] - METADATA_SIZE);
    assert(chunk->size == (uint16_t) 1);
    offset = METADATA_SIZE;
    assert(*((char *) ((uint8_t *) chunk + offset)) == '1');

    /* Validate merge */
    chunk = chunk->next;
    assert(chunk->used == (uint8_t) 0);
    assert(chunk->next == (uint8_t *) ptr[3] - METADATA_SIZE);
    assert(chunk->size == (uint16_t) (METADATA_SIZE + 2) + 3);

    /* Validate tail */
    tail = get_tail();
    assert(tail == chunk->next);
    chunk = tail;
    assert(chunk->used == (uint8_t) 1);
    assert(chunk->next == NULL);
    assert(chunk->size == (uint16_t) 4);
    offset = METADATA_SIZE;
    assert(*((char *) ((uint8_t *) chunk + offset++)) == 'w');
    assert(*((char *) ((uint8_t *) chunk + offset++)) == 'x');
    assert(*((char *) ((uint8_t *) chunk + offset++)) == 'y');
    assert(*((char *) ((uint8_t *) chunk + offset)) == 'z');

    /* Validate brk */
    cur_brk = get_current_brk();
    /* User requested 4 bytes for the last pointer */
    assert(cur_brk == (uint8_t *) tail + METADATA_SIZE + 4);

    /* ---------------------------------------------------------------------- */

    /* Freeing memory before leaving test case
     * Already tested case when
     *   [ U, F, U, ] -> [ F, U ] (merge prev, prev is head)
     *   [ F, U ] -> [ - ] */

    print_sep();
    _malloca_free(ptr[0]); /* [ F, U ] */
    _malloca_free(ptr[3]); /* [ - ] */
    print_sep();

    assert(get_head() == NULL);
    assert(get_tail() == NULL);
    assert(get_current_brk() == (uint8_t *) ptr[0] - METADATA_SIZE);

    return 0;
}
