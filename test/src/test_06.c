#include "../include/common.h"

#define PTR_QNT 4
#define LAST_PTR_IDX (PTR_QNT - 1)
int main(void) {
    void *ptr[PTR_QNT];
    void *tail = NULL;
    chunk_t *chunk = NULL;
    uint8_t offset = 0;

    /* Test case:
     * Merge with next when current pointer being freed is not head.
     * Free userspace size should be:
     * what user asked for previous pointer +
     *           (what asked for current pointer + metadata)
     */

    print_sep();

    _malloca(ptr, 0, 1); /* [ U ] */
    _malloca(ptr, 1, 2); /* [ U, U ] */
    _malloca(ptr, 2, 3); /* [ U, U, U ] */
    _malloca(ptr, 3, 4); /* [ U, U, U, U ] */

    memset((uint8_t *) ptr[0] + 0, '0', 1);

    memset((uint8_t *) ptr[LAST_PTR_IDX] + 0, 'A', 1);
    memset((uint8_t *) ptr[LAST_PTR_IDX] + 1, 'B', 1);
    memset((uint8_t *) ptr[LAST_PTR_IDX] + 2, 'C', 1);
    memset((uint8_t *) ptr[LAST_PTR_IDX] + 3, 'D', 1);

    /* ---------------------------------------------------------------------- */

    _malloca_free(ptr[2]); /* [ U, U, F, U ] */
    _malloca_free(ptr[1]); /* [ U, F, U ] */

    /* Validate head and data integrity */
    chunk = get_head();
    assert(chunk->used == (uint8_t) 1);
    assert(chunk->next ==  (uint8_t *) ptr[1] - METADATA_SIZE);
    assert(chunk->size == 1);
    offset = METADATA_SIZE;
    assert(*((char *) ((uint8_t *) chunk + offset)) == '0');

    /* Validate tail */
    tail = get_tail();
    assert(tail == (uint8_t *) ptr[LAST_PTR_IDX] - METADATA_SIZE);

    /* Validate merge */
    chunk = chunk->next;
    assert(chunk->used == (uint8_t) 0);
    assert(chunk->next == tail);
    assert(chunk->size == 2 + (METADATA_SIZE + 3));

    /* Validate tail data integrity */
    chunk = tail;
    assert(chunk->used == (uint8_t) 1);
    assert(chunk->next == NULL);
    assert(chunk->size == 4);
    offset = METADATA_SIZE;
    assert(*((char *) ((uint8_t *) chunk + offset++)) == 'A');
    assert(*((char *) ((uint8_t *) chunk + offset++)) == 'B');
    assert(*((char *) ((uint8_t *) chunk + offset++)) == 'C');
    assert(*((char *) ((uint8_t *) chunk + offset)) == 'D');

    /* ---------------------------------------------------------------------- */

    /* Freeing memory before leaving test case
     * Already tested case when
     *   [ U, F, U ] -> [ F, U ] (merge prev, prev is head)
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
