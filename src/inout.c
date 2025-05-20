#include "inout.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "objects.h"
#include "terminal.h"

int editor_create(struct editorConfig *conf) {
    if (term_get_window_size(conf, &conf->screen_rows, &conf->screen_cols) !=
        0) {
        return -1;
    }
    return 0;
}

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

int editor_refresh_screen(struct editorConfig *conf) {
    struct abuf ab = ABUF_INIT;
    ab_append(&ab, "\x1b[2J", 4);
    ab_append(&ab, "\x1b[H", 3);

    editor_draw_rows(conf, &ab);
    ab_append(&ab, "\x1b[H", 3);

    write(STDOUT_FILENO, ab.buf, ab.len);
    ab_free(&ab);
    return 0;
}

int editor_draw_rows(struct editorConfig *conf, struct abuf *ab) {
    for (size_t i = 0; i < conf->screen_rows; i++) {
        ab_append(ab, "~", 1);
        if (i < conf->screen_rows - 1) {
            ab_append(ab, "\r\n", 2);
        }
    }

    return 0;
}

char editor_read_key(void) {
    char c;
    int nread;
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if (nread == -1 && errno != EAGAIN) die("read");
    }
    return c;
}

int editor_process_key_press() {
    char c = editor_read_key();
    switch (c) {
        case CTRL_KEY('q'):
            write(STDOUT_FILENO, "\x1b[2J", 4);
            write(STDOUT_FILENO, "\x1b[H", 3);
            exit(1);
            break;
        default:
            printf("%c\r\n", c);
            break;
    }

    return 0;
}