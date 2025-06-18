#include "core.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

// MISC

void die(const char* s) {
    perror(s);
    exit(EXIT_FAILURE);
}

bool is_separator(unsigned char c) {
    return isspace(c) || c == '\0' || strchr(",.()+-/*=~%<>[];", c) != NULL;
}

int count_digits(int n) {
    if (n == 0) return INVALID_ARG;

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
    if (!temp) return OUT_OF_MEMORY;

    memcpy(temp, a, elsize);
    memcpy(a, b, elsize);
    memcpy(b, temp, elsize);

    free(temp);

    return SUCCESS;
}
