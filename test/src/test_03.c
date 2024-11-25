#include "../include/common.h"

int main(void) {
    void *ptr[3];
    const void *initial_brk = syscall_current_brk();
    void *cur_brk = NULL;
    void *head = NULL;
    void *tail = NULL;
    chunk_t *chunk = NULL;

    uint16_t offset = 0;

    assert(sizeof(chunk_t) == 8); /* validate our __packed__ is being respected lol */
    print_sep();

    /* Test case:
     * Merge with previous and previous is head.
     * Free userspace size should be:
     * what user asked for previous pointer +
     *           (what asked for current pointer + metadata)
     */

    _malloca(ptr, 0, 5);    /* [ U ] */
    _malloca(ptr, 1, 6);    /* [ U, U ] */
    _malloca(ptr, 2, 7);    /* [ U, U, U ] */

    /* Initial brk should always have the same value */
    assert(get_initial_brk() == initial_brk);

    memset((uint8_t *) ptr[0] + 0, '1', 1);
    memset((uint8_t *) ptr[0] + 1, '2', 1);
    memset((uint8_t *) ptr[0] + 2, '3', 1);
    memset((uint8_t *) ptr[0] + 3, '4', 1);
    memset((uint8_t *) ptr[0] + 4, '5', 1);

    memset((uint8_t *) ptr[1] + 0, 'a', 1);
    memset((uint8_t *) ptr[1] + 1, 'b', 1);
    memset((uint8_t *) ptr[1] + 2, 'c', 1);
    memset((uint8_t *) ptr[1] + 3, 'd', 1);
    memset((uint8_t *) ptr[1] + 4, 'e', 1);
    memset((uint8_t *) ptr[1] + 5, 'f', 1);

    memset((uint8_t *) ptr[2] + 0, 'A', 1);
    memset((uint8_t *) ptr[2] + 1, 'B', 1);
    memset((uint8_t *) ptr[2] + 2, 'C', 1);
    memset((uint8_t *) ptr[2] + 3, 'D', 1);
    memset((uint8_t *) ptr[2] + 4, 'E', 1);
    memset((uint8_t *) ptr[2] + 5, 'F', 1);
    memset((uint8_t *) ptr[2] + 6, 'G', 1);

    /* Check if malloqueiro returns the right memory region
     * and if we can set user space region properly for all pointers */
    offset = METADATA_SIZE;
    assert(ptr[0] == (void *) ((uint8_t *) initial_brk + offset));
    assert(*((char *) ((uint8_t *) initial_brk + offset++)) == '1');
    assert(*((char *) ((uint8_t *) initial_brk + offset++)) == '2');
    assert(*((char *) ((uint8_t *) initial_brk + offset++)) == '3');
    assert(*((char *) ((uint8_t *) initial_brk + offset++)) == '4');
    assert(*((char *) ((uint8_t *) initial_brk + offset))   == '5');

    /*        5 bytes for the first chunk */
    offset = (5 + METADATA_SIZE) + METADATA_SIZE;
    assert(ptr[1] == (void *) ((uint8_t *) initial_brk + offset));
    assert(*((char *) ((uint8_t *) initial_brk + offset++)) == 'a');
    assert(*((char *) ((uint8_t *) initial_brk + offset++)) == 'b');
    assert(*((char *) ((uint8_t *) initial_brk + offset++)) == 'c');
    assert(*((char *) ((uint8_t *) initial_brk + offset++)) == 'd');
    assert(*((char *) ((uint8_t *) initial_brk + offset++)) == 'e');
    assert(*((char *) ((uint8_t *) initial_brk + offset)) == 'f');

    /*        5 bytes for first and 6 for the second chunk */
    offset = (5 + METADATA_SIZE) + (6 + METADATA_SIZE) + METADATA_SIZE;
    assert(ptr[2] == (void *) ((uint8_t *) initial_brk + offset));
    assert(*((char *) ((uint8_t *) initial_brk + offset++)) == 'A');
    assert(*((char *) ((uint8_t *) initial_brk + offset++)) == 'B');
    assert(*((char *) ((uint8_t *) initial_brk + offset++)) == 'C');
    assert(*((char *) ((uint8_t *) initial_brk + offset++)) == 'D');
    assert(*((char *) ((uint8_t *) initial_brk + offset++)) == 'E');
    assert(*((char *) ((uint8_t *) initial_brk + offset++)) == 'F');
    assert(*((char *) ((uint8_t *) initial_brk + offset++)) == 'G');

    /* Validate head pointer */
    head = get_head();
    assert(head == initial_brk);

    /* Validate tail pointer */
    tail = get_tail();
    assert(tail == (uint8_t *) ptr[2] - METADATA_SIZE);

    /* Validate brk */
    cur_brk = get_current_brk();
    /* User requested 7 bytes for the last pointer */
    assert(cur_brk == (uint8_t *) ptr[2] + 7);

    /* Validate first chunk */
    chunk = head;
    assert(chunk->used == (uint8_t) 1);
    assert(chunk->next == (uint8_t *) ptr[1] - METADATA_SIZE);
    assert(chunk->size == (uint16_t) 5);

    /* Validate second chunk */
    chunk = chunk->next;
    assert(chunk->used == (uint8_t) 1);
    assert(chunk->next == (uint8_t *) ptr[2] - METADATA_SIZE);
    assert(chunk->size == (uint16_t) 6);

    /* Validate third chunk */
    chunk = chunk->next;
    assert(chunk->used == (uint8_t) 1);
    assert(chunk->next == NULL);
    assert(chunk->size == (uint16_t) 7);

    /* ---------------------------------------------------------------------- */

    _malloca_free(ptr[0]);             /* [ F, U, U ] */
    _malloca_free(ptr[1]);             /* [ F, U, ] */

    /* Initial brk should always have the same value */
    assert(get_initial_brk() == initial_brk);

    /* Validate tail */
    tail = get_tail();
    assert(tail == (uint8_t *) ptr[2] - METADATA_SIZE);

    /* Validate merge */
    chunk = head;
    assert(chunk->used == (uint8_t) 0);
    assert(chunk->next == tail);
    assert(chunk->size == (uint16_t) (METADATA_SIZE + 5) + 6);

    /* Validate tail data integrity */
    chunk = tail;
    assert(chunk->used == (uint8_t) 1);
    assert(chunk->next == NULL);
    assert(chunk->size == 7);
    offset = METADATA_SIZE;
    assert(*((char *) ((uint8_t *) chunk + offset++)) == 'A');
    assert(*((char *) ((uint8_t *) chunk + offset++)) == 'B');
    assert(*((char *) ((uint8_t *) chunk + offset++)) == 'C');
    assert(*((char *) ((uint8_t *) chunk + offset++)) == 'D');
    assert(*((char *) ((uint8_t *) chunk + offset++)) == 'E');
    assert(*((char *) ((uint8_t *) chunk + offset++)) == 'F');
    assert(*((char *) ((uint8_t *) chunk + offset++)) == 'G');

    /* Validate brk */
    cur_brk = get_current_brk();
    /* User requested 7 bytes for the last pointer */
    assert(cur_brk == (uint8_t *) tail + METADATA_SIZE + 7);

    /* ---------------------------------------------------------------------- */

    /* Freeing memory before leaving test case
     * Already tested case when [ U ] -> [ - ] */

    print_sep();
    _malloca_free(ptr[2]); /* [ - ] */
    print_sep();

    assert(get_head() == NULL);
    assert(get_tail() == NULL);
    assert(get_current_brk() == (uint8_t *) ptr[0] - METADATA_SIZE);

    return 0;
}
