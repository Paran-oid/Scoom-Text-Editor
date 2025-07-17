#ifndef BUFFER_H
#define BUFFER_H

#include <stddef.h>
#include <stdint.h>

struct ABuf {
    char *buf;
    int32_t len;
};

#define ABUF_INIT \
    { NULL, 0 }

int8_t ab_append(struct ABuf *ab, const char *s, int32_t len);
int8_t ab_free(struct ABuf *ab);

#endif