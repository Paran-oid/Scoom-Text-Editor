#include "buffer.h"

#include <stdlib.h>
#include <string.h>

int ab_append(struct ABuf *ab, const char *s, size_t slen) {
    char *new = realloc(ab->buf, ab->len + slen);
    if (!new) return -1;

    memcpy(&new[ab->len], s, slen);
    ab->buf = new;
    ab->len += slen;

    return EXIT_SUCCESS;
}

int ab_free(struct ABuf *ab) {
    free(ab->buf);
    return EXIT_SUCCESS;
}
