#include "buffer.h"

#include <stdlib.h>
#include <string.h>

#include "config.h"

int ab_append(struct ABuf *ab, const char *s, size_t slen) {
    char *new = realloc(ab->buf, ab->len + slen);
    if (!new) return -1;

    memcpy(&new[ab->len], s, slen);
    ab->buf = new;
    ab->len += slen;

    return SUCCESS;
}

int ab_free(struct ABuf *ab) {
    free(ab->buf);
    ab->buf = NULL;
    return SUCCESS;
}
