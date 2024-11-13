#include <stdio.h>

int main() {
    /* perror uses malloc and free (on my machine lol)
     *
     * $ uname -a
     * Linux machine 6.1.0-26-amd64 #1 SMP PREEMPT_DYNAMIC Debian 6.1.112-1 (2024-09-30) x86_64 GNU/Linux
     * */

    perror("foo bar");
    return 0;
}
