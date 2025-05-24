#include "inout.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "file.h"
#include "objects.h"
#include "terminal.h"

/***  Misc ***/

int editor_create(struct Config *conf) {
    // cursor section
    conf->cx = 0;
    conf->cy = 0;
    conf->rx = 0;

    conf->numrows = 0;
    conf->rows = NULL;
    conf->rowoff = 0;
    conf->coloff = 0;
    if (term_get_window_size(conf, &conf->screen_rows, &conf->screen_cols) !=
        0) {
        return -1;
    }

    conf->screen_rows--;
    return 0;
}

/***  Appending buffer section ***/

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

/***  Screen display and rendering section ***/

int editor_refresh_screen(struct Config *conf) {
    editor_scroll(conf);

    struct abuf ab = ABUF_INIT;
    ab_append(&ab, "\x1b[?25l", 6);  // hide cursor
    ab_append(&ab, "\x1b[H", 3);     // move top left

    editor_draw_rows(conf, &ab);
    editor_draw_statusbar(conf, &ab);

    ab_append(&ab, "\x1b[H", 3);
    ab_append(&ab, "\x1b[?25h", 6);  // display cursor again

    /*
            Cursor is 1-indexed, so we have to also add 1 for it's coordinates
       to be valid
    */

    char buf[32];
    // <esc>[<row>;<col>H
    snprintf(buf, sizeof(buf), "\x1B[%d;%dH", conf->cy - conf->rowoff + 1,
             conf->rx - conf->coloff + 1);
    ab_append(&ab, buf, strlen(buf));

    write(STDOUT_FILENO, ab.buf, ab.len);
    ab_free(&ab);
    return 0;
}

int editor_draw_statusbar(struct Config *conf, struct abuf *ab) {
    /*
     you could specify all of these attributes using the command <esc>[1;4;5;7m.
     An argument of 0 clears all attributes, and is the default argument, so we
     use <esc>[m to go back to normal text formatting.
    */

    ab_append(ab, "\x1b[7m", 4);  // invert colors
    size_t len = 0;
    while (len < conf->screen_cols) {
        ab_append(ab, " ", 1);
        len++;
    }
    ab_append(ab, "\x1b[m", 3);  // returns to normal
}

int editor_draw_rows(struct Config *conf, struct abuf *ab) {
    for (size_t y = 0; y < conf->screen_rows; y++) {
        int filerow = y + conf->rowoff;
        if (filerow >= conf->numrows) {
            // if there are no rows then we can safely assume we need to output
            // welcome screen
            if (conf->numrows == 0 && y == conf->screen_rows / 3) {
                char buf[100];
                int welcome_len = snprintf(buf, sizeof(buf),
                                           "Welcome to version %.2lf of Scoom!",
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
        } else {
            int len = conf->rows[filerow].rsize - conf->coloff;
            if (len < 0) len = 0;
            if (len > conf->screen_cols) {
                len = conf->screen_cols;
            }
            ab_append(ab, &conf->rows[filerow].render[conf->coloff], len);
        }

        ab_append(ab, "\x1b[K", 4);  // erase in line command
        ab_append(ab, "\r\n", 2);
    }

    return 0;
}

/***  Cursor movement and scrolling section ***/

int editor_scroll(struct Config *conf) {
    conf->rx = 0;
    if (conf->cy < conf->numrows) {
        conf->rx = editor_update_cx_rx(&conf->rows[conf->cy], conf->cx);
    }

    if (conf->rx < conf->coloff) {
        conf->coloff = conf->rx;
    }

    if (conf->rx >= conf->screen_cols + conf->coloff) {
        conf->coloff = conf->rx - conf->screen_cols + 1;
    }

    if (conf->cy < conf->rowoff) {
        conf->rowoff = conf->cy;
    }
    if (conf->cy >= conf->rowoff + conf->screen_rows) {
        conf->rowoff = conf->cy - conf->screen_rows + 1;
    }
    return 0;
}

int editor_cursor_move(struct Config *conf, int key) {
    struct e_row *row =
        (conf->cy >= conf->numrows) ? NULL : &conf->rows[conf->cy];

    switch (key) {
        case ARROW_LEFT:
            if (conf->cx != 0) {
                conf->cx--;
            } else if (conf->cy > 0) {
                conf->cy--;
                conf->cx = conf->rows[conf->cy].size;
            }
            break;
        case ARROW_RIGHT:
            if (row && conf->cx < row->size) {
                conf->cx++;
            } else if (row && conf->cy < conf->numrows) {
                conf->cx = 0;
                conf->cy++;
            }
            break;
        case ARROW_UP:
            if (conf->cy != 0) conf->cy--;
            break;
        case ARROW_DOWN:
            if (conf->cy < conf->numrows) conf->cy++;
            break;
        default:
            return 1;
    }

    row = (conf->cy >= conf->numrows) ? NULL : &conf->rows[conf->cy];
    if (row && conf->cx > row->size) {
        conf->cx = row->size;
    }

    return 0;
}

/***  Key processing section ***/

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
            if ('0' <= seq[1] && seq[1] <= '9') {
                if (read(STDIN_FILENO, &seq[2], 1) != 1) return '\x1b';
                if (seq[2] == '~') {
                    switch (seq[1]) {
                        case '1':
                        case '7':
                            return HOME_KEY;
                        case '3':
                            return DEL_KEY;
                        case '4':
                        case '8':
                            return END_KEY;
                        case '5':
                            return PAGE_UP;
                        case '6':
                            return PAGE_DOWN;
                    }
                }
            }
            switch (seq[1]) {
                case 'A':
                    return ARROW_UP;
                case 'B':
                    return ARROW_DOWN;
                case 'C':
                    return ARROW_RIGHT;
                case 'D':
                    return ARROW_LEFT;
                case 'H':
                    return HOME_KEY;
                case 'F':
                    return END_KEY;
            }
        } else if (seq[0] == 'O') {
            switch (seq[1]) {
                case 'H':
                    return HOME_KEY;
                    break;
                case 'F':
                    return END_KEY;
                    break;
            }
        } else {
            return '\x1b';
        }
    } else {
        return c;
    }
}

int editor_process_key_press(struct Config *conf) {
    /*
                Side Note:
                        We use int c instead of char c since we have mapped some
                        keys to values bigger than the max 255
                        like for ARROWS

    */
    struct e_row *row =
        (conf->cy >= conf->numrows) ? NULL : &conf->rows[conf->cy];
    int c = editor_read_key();

    int times = conf->screen_rows;  // this will be needed in case of page up or
                                    // down basically
    switch (c) {
        case ARROW_UP:
        case ARROW_DOWN:
        case ARROW_RIGHT:
        case ARROW_LEFT:
            editor_cursor_move(conf, c);
            break;
        case PAGE_UP:
        case PAGE_DOWN:
            if (c == PAGE_UP) {
                // cursor jumps up to previous page
                conf->cy = conf->rowoff;
            } else if (c == PAGE_DOWN) {
                // cursor jumps up to next page
                conf->cy = conf->rowoff + conf->screen_rows - 1;
                if (conf->cy > conf->numrows) conf->cy = conf->numrows;
            }
            while (times--) {
                editor_cursor_move(conf, c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
            }
            break;
        case HOME_KEY:
            conf->cx = 0;
            break;
        case END_KEY:
            if (row) {
                conf->cx = row->size;
            }
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