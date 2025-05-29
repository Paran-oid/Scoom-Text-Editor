#include "core.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// MISC

void die(const char* s) {
    perror(s);
    exit(1);
}

int swap(void* a, void* b, size_t elsize) {
    void* temp = malloc(elsize);

    memcpy(temp, a, elsize);
    memcpy(a, b, elsize);
    memcpy(b, temp, elsize);

    free(temp);

    return 0;
}