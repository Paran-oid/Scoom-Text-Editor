#include "inout.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "objects.h"
#include "terminal.h"

int editor_create(struct editorConfig *conf) {
    conf->cx = 0;
    conf->cy = 0;

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
    ab_append(&ab, "\x1b[?25l", 6);
    ab_append(&ab, "\x1b[H", 3);  // move top left

    editor_draw_rows(conf, &ab);
    ab_append(&ab, "\x1b[H", 3);
    ab_append(&ab, "\x1b[?25h", 6);

    /*
            Cursor is 1-indexed, so we have to also add 1 for it's coordinates
       to be valid
    */

    char buf[32];
    // <esc>[<row>;<col>H
    snprintf(buf, sizeof(buf), "\x1B[%d;%dH", conf->cy + 1, conf->cx + 1);
    ab_append(&ab, buf, strlen(buf));

    write(STDOUT_FILENO, ab.buf, ab.len);
    ab_free(&ab);
    return 0;
}

int editor_draw_rows(struct editorConfig *conf, struct abuf *ab) {
    for (size_t y = 0; y < conf->screen_rows; y++) {
        if (y == conf->screen_rows / 3) {
            char buf[100];
            int welcome_len =
                snprintf(buf, sizeof(buf), "Welcome to version %.2lf of Scoom!",
                         SCOOM_VERSION);

            if (welcome_len > conf->screen_cols)
                welcome_len = conf->screen_cols;

            int padding = (conf->screen_cols - welcome_len) / 2;
            if (padding) {
                ab_append(ab, "~", 1);
                padding--;
            }

            while (padding--) ab_append(ab, " ", 1);

            ab_append(ab, buf, welcome_len);
        } else {
            ab_append(ab, "~", 1);
        }
        ab_append(ab, "\x1b[K", 4);  // erase in line command
        if (y < conf->screen_rows - 1) {
            ab_append(ab, "\r\n", 2);
        }
    }

    return 0;
}

void editor_cursor_move(struct editorConfig *conf, int key) {
    switch (key) {
        case ARROW_LEFT:
            if (conf->cx != 0) conf->cx--;
            break;
        case ARROW_RIGHT:
            if (conf->cx != conf->screen_cols - 1) conf->cx++;
            break;
        case ARROW_UP:
            if (conf->cy != 0) conf->cy--;
            break;
        case ARROW_DOWN:
            if (conf->cy != conf->screen_rows - 1) conf->cy++;
            break;
    }
}

int editor_read_key(void) {
    char c;
    int nread;
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if (nread == -1 && errno != EAGAIN) die("read");
    }

    if (c == '\x1b') {
        char seq[3];
        if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
        if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';
        if (seq[0] == '[') {
            switch (seq[1]) {
                case 'A':
                    return ARROW_UP;
                case 'B':
                    return ARROW_DOWN;
                case 'C':
                    return ARROW_RIGHT;
                case 'D':
                    return ARROW_LEFT;
            }
        } else {
            return '\x1b';
        }
    } else {
        return c;
    }
}

int editor_process_key_press(struct editorConfig *conf) {
    /*
                Side Note:
                        We use int c instead of char c since we have mapped some
                        keys to values bigger than the max 255
                        like for ARROWS

    */
    int c = editor_read_key();
    switch (c) {
        case ARROW_UP:
        case ARROW_DOWN:
        case ARROW_RIGHT:
        case ARROW_LEFT:
            editor_cursor_move(conf, c);
            break;
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