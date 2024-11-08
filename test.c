
#include <stddef.h>

void *malloca(size_t bytes); /* bytes must be <= 0x00FFFFFF*/

int main() {
    malloca(1);
    malloca(0xffff);
    malloca(0xffff + 1);
    /*0xFFFFFFFF*/
    /*printf("Got %p from malloca\n\n", malloca(4294967295u));
    printf("Got %p from malloca\n\n", malloca(0xFFFFFFFF));*/
  return 0;
}
