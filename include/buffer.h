#ifndef BUFFER_H
#define BUFFER_H

#include <stddef.h>
struct abuf {
    char *buf;
    int len;
};

int ab_append(struct abuf *ab, const char *s, size_t len);
int ab_free(struct abuf *ab);

#endif