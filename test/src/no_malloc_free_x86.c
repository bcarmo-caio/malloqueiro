#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <fcntl.h>

void *malloc(__attribute__ ((unused)) size_t size) {
    char msg[] = "Error: malloc is not allowed\n";
    int fd = open("/dev/stdout", O_WRONLY);
    write(fd, msg, sizeof(msg));
    close(fd);
    abort();
}

void free(__attribute__ ((unused)) void *ptr) {
    char msg[] = "Error: free is not allowed\n";
    int fd = open("/dev/stdout", O_WRONLY);
    write(fd, msg, sizeof(msg));
    close(fd);
    abort();
}
