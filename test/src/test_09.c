#include "../include/common.h"

int main(void) {
    void *ptr[3];
    void *initial_brk = syscall_current_brk();
    chunk_t *chunk = NULL;

    assert(sizeof(chunk_t) == 8); /* validate our __packed__ is being respected lol */
    print_sep();

    /* Test case:
     * Allocate 2 chunks,
     *   free the first (head)
     *   asks for another one the same size we just freed
     * Malloqueiro should return the previous freed one (head)
     * */

    /* ---------------------------------------------------------------------- */

    _malloca(ptr, 0, 15); /* [ U ]    */
    _malloca(ptr, 1, 2);  /* [ U, U ] */
    memset((uint8_t *) ptr[1] + 0, 'A', 1);
    memset((uint8_t *) ptr[1] + 1, 'B', 1);
    _malloca_free(ptr[0]); /* [ F, U ]*/
    _malloca(ptr, 2, 15); /* [ U, U ] */

    /* Assert newly allocated pointer is the same as previously freed one */
    assert(ptr[0] == ptr[2]);

    /* Validate head */
    chunk = (chunk_t *) initial_brk;
    assert(chunk->used == (uint8_t) 1);
    assert(chunk->next == (uint8_t *) ptr[1] - METADATA_SIZE);
    assert(chunk->size == (uint16_t) 15);

    /* Validate tail data integrity */
    chunk = chunk->next;
    assert(* (char *) ( (uint8_t *) ptr[1] + 0 ) == 'A');
    assert(* (char *) ( (uint8_t *) ptr[1] + 1 ) == 'B');
    assert(chunk->next == NULL);


    /* ---------------------------------------------------------------------- */

    /* Freeing memory before leaving test case
     * Already tested case when
     *   [ U, U ] -> [ U ]
     *   [ U ] -> [ - ] */

    print_sep();
    _malloca_free(ptr[1]); /* [ U ] */
    _malloca_free(ptr[2]); /* [ - ] */
    print_sep();

    /* Validate everything got reset */
    assert(get_initial_brk() == initial_brk);
    assert(get_current_brk() == initial_brk);
    assert(get_head() == NULL);
    assert(get_tail() == NULL);

    return 0;
}
