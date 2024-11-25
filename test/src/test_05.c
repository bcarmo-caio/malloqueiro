#include "../include/common.h"

int main(void) {
    void *ptr[3];
    void *tail = NULL;
    chunk_t *chunk = NULL;
    uint16_t offset = 0;

    /* Test case:
     * Merge with next when freeing head.
     * Free userspace size should be:
     * what user asked for previous pointer +
     *           (what asked for current pointer + metadata)
     */
    print_sep();

    _malloca(ptr, 0, 1);    /* [ U ] */
    _malloca(ptr, 1, 2);    /* [ U, U ] */
    _malloca(ptr, 2, 3);    /* [ U, U, U ] */

    memset((uint8_t *) ptr[0] + 0, '1', 1);

    memset((uint8_t *) ptr[1] + 0, 'a', 1);
    memset((uint8_t *) ptr[1] + 1, 'b', 1);

    memset((uint8_t *) ptr[2] + 0, 'A', 1);
    memset((uint8_t *) ptr[2] + 1, 'B', 1);
    memset((uint8_t *) ptr[2] + 2, 'C', 1);

    /* ---------------------------------------------------------------------- */

    _malloca_free(ptr[1]);             /* [ U, F, U ] */
    _malloca_free(ptr[0]);             /* [ F, U, ] */

    /* Validate tail */
    tail = get_tail();
    assert(tail == (uint8_t *) ptr[2] - METADATA_SIZE);

    /* Validate merge */
    chunk = get_head();
    assert(chunk->used == (uint8_t) 0);
    assert(chunk->next == tail);
    assert(chunk->size == (uint16_t) (METADATA_SIZE + 1) + 2);

    /* Validate tail data integrity */
    chunk = tail;
    assert(chunk->used == (uint8_t) 1);
    assert(chunk->next == NULL);
    assert(chunk->size == 3);
    offset = METADATA_SIZE;
    assert(*((char *) ((uint8_t *) chunk + offset++)) == 'A');
    assert(*((char *) ((uint8_t *) chunk + offset++)) == 'B');
    assert(*((char *) ((uint8_t *) chunk + offset)) == 'C');

    /* ---------------------------------------------------------------------- */

    /* Freeing memory before leaving test case
     * Already tested case when [ F, U ] -> [ - ] */

    print_sep();
    _malloca_free(ptr[2]); /* [ - ] */
    print_sep();

    assert(get_head() == NULL);
    assert(get_tail() == NULL);
    assert(get_current_brk() == (uint8_t *) ptr[0] - METADATA_SIZE);

    return 0;
}
