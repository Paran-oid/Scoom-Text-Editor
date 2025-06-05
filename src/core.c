#include "core.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// MISC

void die(const char* s) {
    perror(s);
    exit(1);
}

bool is_separator(unsigned char c) {
    return isspace(c) || c == '\0' || strchr(",.()+-/*=~%<>[];", c) != NULL;
}

int count_digits(int n) {
    if (n == 0) return 1;

    int res = 0;
    while (n) {
        n /= 10;
        res++;
    }
    return res;
}

// MEMORY

int swap(void* a, void* b, size_t elsize) {
    void* temp = malloc(elsize);

    memcpy(temp, a, elsize);
    memcpy(a, b, elsize);
    memcpy(b, temp, elsize);

    free(temp);

    return 0;
}
