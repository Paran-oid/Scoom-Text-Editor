#include "buffer.h"

#include <stdlib.h>
#include <string.h>

#include "core.h"

int ab_append(struct ABuf *ab, const char *s, size_t slen) {
    char *new = realloc(ab->buf, ab->len + slen);
    if (!new) die("realloc failed, out of memory");

    memcpy(&new[ab->len], s, slen);
    ab->buf = new;
    ab->len += slen;

    return EXIT_SUCCESS;
}

int ab_free(struct ABuf *ab) {
    free(ab->buf);
    ab->buf = NULL;
    return EXIT_SUCCESS;
}
