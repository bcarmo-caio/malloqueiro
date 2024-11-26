#include "../include/common.h"

int main(void) {
    void *ptr[3];
    void *initial_brk = syscall_current_brk();
    chunk_t *chunk = NULL;

    assert(sizeof(chunk_t) == 8); /* validate our __packed__ is being respected lol */
    print_sep();

    /* Test case:
     * Allocate 2 chunks,
     *   First one (named A) with n bytes being n = 2 + metadata + 1
     *   Second one with any size.
     *   Free A, then
     *     for m in {1, 2}, do
     *       ask for m bytes
     *       free this new allocated space
     * Malloqueiro should always split A into two chunks
     *  One sizing m and other sizing n - m - metadata
     *
     * When allocating n = 3, we should get the whole A, i.e. A should
     *   not be split.
     * */

    /* ---------------------------------------------------------------------- */

    _malloca(ptr, 0, 2 + METADATA_SIZE + 1); /* [ U ]    */
    _malloca(ptr, 1, 1);  /* [ U, U ] */
    memset(ptr[1], 'B', 1);

    assert(METADATA_SIZE == 8);

    /* ---------------------------------------------------------------------- */

    /* Asking for 1 byte. Should split into two chunks (1 byte and 2 bytes) */
    _malloca_free(ptr[0]); /* [ F, U ] */
    _malloca(ptr, 0,  1); /* [ U, F, U ] */

    /* Validate return value */
    assert(ptr[0] == (uint8_t *) initial_brk + METADATA_SIZE);

    /* Validate chunk A (head) metadata */
    chunk = initial_brk;
    assert(chunk->used == (uint8_t) 1);
    assert(chunk->next == (uint8_t *) ptr[0] + 1);
    assert(chunk->size == (uint16_t) 1);

    /* Validate chunk B metadata */
    chunk = chunk->next;
    assert(chunk->used == (uint8_t) 0);
    assert(chunk->next == (uint8_t *) ptr[1] - METADATA_SIZE);
    assert(chunk->size == (uint16_t) 2);

    /* Validate tail metadata and data integrity */
    chunk = chunk->next;
    assert(chunk->used == (uint8_t) 1);
    assert(chunk->next == NULL);
    assert(chunk->size == (uint16_t) 1);
    assert(* (char *) ( (uint8_t *) ptr[1] + 0 ) == 'B');

    /* ---------------------------------------------------------------------- */

    /* Asking for 2 byte. Should split into two chunks (2 byte and 1 bytes) */
    _malloca_free(ptr[0]); /* [ F, U ] */
    _malloca(ptr, 0,  2); /* [ U, F, U ] */

    /* Validate return value */
    assert(ptr[0] == (uint8_t *) initial_brk + METADATA_SIZE);

    /* Validate chunk A (head) metadata */
    chunk = initial_brk;
    assert(chunk->used == (uint8_t) 1);
    assert(chunk->next == (uint8_t *) ptr[0] + 2);
    assert(chunk->size == (uint16_t) 2);

    /* Validate chunk B  metadata */
    chunk = chunk->next;
    assert(chunk->used == (uint8_t) 0);
    assert(chunk->next == (uint8_t *) ptr[1] - METADATA_SIZE);
    assert(chunk->size == (uint16_t) 1);

    /* Validate tail metadata and data integrity */
    chunk = chunk->next;
    assert(chunk->used == (uint8_t) 1);
    assert(chunk->next == NULL);
    assert(chunk->size == (uint16_t) 1);
    assert(* (char *) ( (uint8_t *) ptr[1] + 0 ) == 'B');

    /* ---------------------------------------------------------------------- */

    /* Asking for 3 byte. Should return the first chunk */
    _malloca_free(ptr[0]); /* [ F, U ] */
    _malloca(ptr, 2,  3); /* [ U, U ] */

    /* Validate return value */
    assert(ptr[0] == ptr[2]);

    /* Validate head metadata */
    chunk = initial_brk;
    assert(chunk->used == (uint8_t) 1);
    assert(chunk->next == (uint8_t *) ptr[1] - METADATA_SIZE);
    assert(chunk->size == (uint16_t) (2 + METADATA_SIZE + 1));

    /* Validate tail metadata and data integrity */
    chunk = chunk->next;
    assert(chunk->used == (uint8_t) 1);
    assert(chunk->next == NULL);
    assert(chunk->size == (uint16_t) 1);
    assert(* (char *) ( (uint8_t *) ptr[1] + 0 ) == 'B');

    /* ---------------------------------------------------------------------- */

    /* Freeing memory before leaving test case
     * Already tested case when
     *   [ U, U ] -> [ U ]
     *   [ U ] -> [ - ] */

    print_sep();
    _malloca_free(ptr[1]); /* [ U ] */
    _malloca_free(ptr[0]); /* [ - ] */
    print_sep();

    /* Validate everything got reset */
    assert(get_initial_brk() == initial_brk);
    assert(get_current_brk() == initial_brk);
    assert(get_head() == NULL);
    assert(get_tail() == NULL);

    return 0;
}
