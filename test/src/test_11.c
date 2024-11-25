#include "../include/common.h"

int main(void) {
    void *ptr[3];
    void *initial_brk = syscall_current_brk();
    uint8_t i;
    chunk_t *chunk = NULL;

    assert(sizeof(chunk_t) == 8); /* validate our __packed__ is being respected lol */
    print_sep();

    /* Test case:
     * Allocate 2 chunks,
     *   First one (named A) with n bytes being n = metadata + 1
     *   Second one with any size.
     *   Free A, then
     *     for m in {1, 2, 3, ... metadata + 1}, do
     *       ask for m bytes
     *       free this new allocated space
     * Malloqueiro should always return A because we cannot split
     * A into 2 chunks because a chunk must have at least the following size
     *   metadata (for metadata) + 1 (minimum requested size allowed)
     * */

    /* ---------------------------------------------------------------------- */

    _malloca(ptr, 0, METADATA_SIZE + 1); /* [ U ]    */
    _malloca(ptr, 1, 1);  /* [ U, U ] */
    memset(ptr[1], 'B', 1);

    assert(METADATA_SIZE == 8);

    for(i = 1; i <= METADATA_SIZE + 1; i++) {
        _malloca_free(ptr[0]); /* [ F, U ]*/
        _malloca(ptr, 0, i); /* [ U, U ] */

        assert((uint8_t *) ptr[0] - METADATA_SIZE == initial_brk);

        /* Validate head metadata */
        chunk = initial_brk;
        assert(chunk->used == (uint8_t) 1);
        assert(chunk->next == (uint8_t *) ptr[1] - METADATA_SIZE);
        assert(chunk->size == (uint16_t) METADATA_SIZE + 1);

        /* Validate tail data integrity */
        chunk = chunk->next;
        assert(chunk->used == (uint8_t) 1);
        assert(chunk->next == NULL);
        assert(chunk->size == (uint16_t) 1);
        assert(* (char *) ( (uint8_t *) ptr[1] + 0 ) == 'B');
    }

    /* Last allocated size was 9, next is 10 and must not be
     * the same chunk. Must change brk */
    assert(i == 10);

    _malloca_free(ptr[0]); /* [ F, U ]*/
    _malloca(ptr, 2, 10); /* [ F, U, U ] */

    chunk = (chunk_t *) initial_brk;
    assert(chunk->used == (uint8_t) 0);
    assert(chunk->next == (uint8_t *) ptr[1] - METADATA_SIZE);
    assert(chunk->size == 9);

    chunk = chunk->next;
    assert(chunk->used == (uint8_t) 1);
    assert(* (char *) ( (uint8_t *) ptr[1] + 0 ) == 'B');
    assert(chunk->size == 1);

    assert(chunk->next != NULL);
    chunk = chunk->next;

    assert(chunk->used == (uint8_t) 1);
    assert(chunk->next == NULL);
    assert(chunk->size == 10);

    /* ---------------------------------------------------------------------- */

    /* Freeing memory before leaving test case
     * Already tested case when
     *   [ F, U, U ] -> [ U, U ]
     *   [ U, U ] -> [ U ]
     *   [ U ] -> [ - ] */

    print_sep();
    _malloca_free(ptr[2]); /* [ U ] */
    _malloca_free(ptr[1]); /* [ - ] */
    print_sep();

    /* Validate everything got reset */
    assert(get_initial_brk() == initial_brk);
    assert(get_current_brk() == initial_brk);
    assert(get_head() == NULL);
    assert(get_tail() == NULL);

    return 0;
}
