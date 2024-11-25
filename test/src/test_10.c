#include "../include/common.h"

int main(void) {
    void *ptr[4];
    void *initial_brk = syscall_current_brk();
    chunk_t *chunk = NULL;

    assert(sizeof(chunk_t) == 8); /* validate our __packed__ is being respected lol */
    print_sep();

    /* Test case:
     * Allocate 3 chunks,
     *   free the second one first (not head)
     *   asks for another one the same size we just freed
     * Malloqueiro should return the previous freed one
     * */

    /* ---------------------------------------------------------------------- */

    _malloca(ptr, 0, 2); /* [ U ]    */
    _malloca(ptr, 1, 3); /* [ U, U ] */
    _malloca(ptr, 2, 4); /* [ U, U, U ] */
    memset((uint8_t *) ptr[0] + 0, '6', 1);
    memset((uint8_t *) ptr[0] + 1, '7', 1);

    memset((uint8_t *) ptr[2] + 0, 'q', 1);
    memset((uint8_t *) ptr[2] + 1, 'w', 1);
    memset((uint8_t *) ptr[2] + 2, 'e', 1);

    _malloca_free(ptr[1]); /* [ U, F, U ] */
    _malloca(ptr, 3, 3); /* [ U, U, U ] */

    /* Assert newly allocated pointer is the same as previously freed one */
    assert(ptr[1] == ptr[3]);

    /* Validate head and data integrity */
    chunk = (chunk_t *) initial_brk;
    assert(chunk->used == (uint8_t) 1);
    assert(chunk->next == (uint8_t *) ptr[3] - METADATA_SIZE);
    assert(chunk->size == (uint16_t) 2);
    assert(* (char *) ( (uint8_t *) ptr[0] + 0 ) == '6');
    assert(* (char *) ( (uint8_t *) ptr[0] + 1 ) == '7');

    /* Validate reallocated chunk metadata */
    chunk = chunk->next;
    assert(chunk->used == (uint8_t) 1);
    assert(chunk->next == (uint8_t *) ptr[2] - METADATA_SIZE);
    assert(chunk->size == (uint16_t) 3);

    /* Validate tail data integrity */
    chunk = chunk->next;
    assert(chunk->used == (uint8_t) 1);
    assert(chunk->next == NULL);
    assert(chunk->size == (uint16_t) 4);
    assert(* (char *) ( (uint8_t *) ptr[2] + 0 ) == 'q');
    assert(* (char *) ( (uint8_t *) ptr[2] + 1 ) == 'w');
    assert(* (char *) ( (uint8_t *) ptr[2] + 2 ) == 'e');

    /* ---------------------------------------------------------------------- */

    /* Freeing memory before leaving test case
     * Already tested case when
     *   [ U, U, U ] -> [ U, F, U ]
     *   [ U, F, U ] -> [ F, U ] (merge prev, prev is head)
     *   [ F, U ] -> [ - ] */

    print_sep();
    _malloca_free(ptr[3]); /* [ U, F, U ] */
    _malloca_free(ptr[0]); /* [ F, U ] */
    _malloca_free(ptr[2]); /* [ - ] */
    print_sep();

    /* Validate everything got reset */
    assert(get_initial_brk() == initial_brk);
    assert(get_current_brk() == initial_brk);
    assert(get_head() == NULL);
    assert(get_tail() == NULL);

    return 0;
}
