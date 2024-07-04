#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdarg.h>
#include "buffered_open.h"

int main() {
    buffered_file_t *bf = buffered_open("input_file.txt", O_RDWR , 0644);
    if (!bf) {
        perror("buffered_open");
        return 1;
    }

    char into[8194];
    int toWrite = buffered_read(bf, into, 8193);
    if (toWrite == -1) {
        perror("buffered_read");
        buffered_close(bf);
        return 1;
    }

    if (buffered_close(bf) == -1) {
        perror("buffered_close");
        return 1;
    }

    buffered_file_t *newBf = buffered_open("output_file.txt", O_RDWR | O_PREAPPEND, 0644);
    if (!newBf) {
        perror("buffered_open");
        return 1;
    }
    
    int written = buffered_write(newBf, into, toWrite);
    if (toWrite != written) {
        perror("buffered_read");
        buffered_close(newBf);
        return 1;
    }

    if (buffered_close(newBf) == -1) {
        perror("buffered_close");
        return 1;
    }

    return 0;
}