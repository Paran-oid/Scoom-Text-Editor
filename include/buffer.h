#ifndef BUFFER_H
#define BUFFER_H

#include <stddef.h>
#include <stdint.h>

struct ABuf {
    char *buf;
    size_t len;
};

#define ABUF_INIT \
    { NULL, 0 }

uint8_t ab_append(struct ABuf *ab, const char *s, size_t len);
uint8_t ab_free(struct ABuf *ab);

#endif