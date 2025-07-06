#ifndef BUFFER_H
#define BUFFER_H

#include <stddef.h>

struct ABuf {
    char *buf;
    int len;
};

#define ABUF_INIT \
    { NULL, 0 }

int ab_append(struct ABuf *ab, const char *s, size_t len);
int ab_free(struct ABuf *ab);

#endif