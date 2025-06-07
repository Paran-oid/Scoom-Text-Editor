#include "buffer.h"

#include <stdlib.h>
#include <string.h>

int ab_append(struct abuf *ab, const char *s, size_t slen) {
    char *new = realloc(ab->buf, sizeof(char) * (ab->len + slen));
    if (!new) return -1;

    memcpy(&new[ab->len], s, slen);
    ab->buf = new;
    ab->len += slen;

    return 0;
}

int ab_free(struct abuf *ab) {
    free(ab->buf);
    return 0;
}
