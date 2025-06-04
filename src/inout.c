#include "inout.h"

#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "core.h"
#include "file.h"
#include "highlight.h"
#include "objects.h"
#include "rows.h"
#include "terminal.h"
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

char *editor_prompt(struct Config *conf, const char *prompt,
                    void (*callback)(struct Config *, char *, int)) {
    size_t bufsize = 128;
    size_t buflen = 0;
    char *buf = malloc(bufsize);
    buf[0] = '\0';

    while (1) {
        editor_set_status_message(conf, prompt, buf);
        editor_refresh_screen(conf);
        int c = editor_read_key();

        if (c == '\r') {
            if (buflen != 0) {
                editor_set_status_message(conf, "");
                if (callback) callback(conf, buf, c);
                return buf;
            }

        } else if (c == '\x1b') {
            editor_set_status_message(conf, "");
            if (callback) callback(conf, buf, c);
            free(buf);
            return NULL;
        } else if (!iscntrl(c) && c < 128) {
            // if the character is not a control character and not one of the
            // mapped Keys from objects.h
            if (buflen == bufsize - 1) {
                // resize buf essentially
                bufsize *= 2;
                buf = realloc(buf, bufsize);
            }
            buf[buflen++] = c;
            buf[buflen] = '\0';
        } else if (c == DEL_KEY || c == CTRL_KEY('h') || c == BACKSPACE) {
            if (buflen != 0) {
                buf[--buflen] = '\0';
            }
        }

        if (callback) callback(conf, buf, c);
    }
    return NULL;
}

int editor_set_status_message(struct Config *conf, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(conf->sbuf, sizeof(conf->sbuf), fmt, ap);
    va_end(ap);
    conf->sbuf_time = time(NULL);

    return 0;
}

/***  Screen display and rendering section ***/

int editor_refresh_screen(struct Config *conf) {
    editor_scroll(conf);

    struct abuf ab = ABUF_INIT;
    ab_append(&ab, "\x1b[6 q", 5);
    ab_append(&ab, "\x1b[?25l", 6);  // hide cursor
    ab_append(&ab, "\x1b[H", 3);     // move top left

    editor_draw_rows(conf, &ab);
    editor_draw_statusbar(conf, &ab);
    editor_draw_messagebar(conf, &ab);

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

    if (write(STDOUT_FILENO, ab.buf, ab.len) == 0) {
        return 1;
    }

    ab_free(&ab);
    return 0;
}

int editor_draw_messagebar(struct Config *conf, struct abuf *ab) {
    ab_append(ab, "\x1b[K", 3);  // we clear current line in terminal
    size_t message_len = strlen(conf->sbuf);
    if (message_len > (size_t)conf->screen_cols)
        message_len = conf->screen_cols;
    // if message has length and time elapsed since
    // last time message inserted is bigger than 5
    if (message_len && time(NULL) - conf->sbuf_time < 5)
        ab_append(ab, conf->sbuf, message_len);

    return 0;
}

int editor_draw_statusbar(struct Config *conf, struct abuf *ab) {
    /*
     you could specify all of these attributes using the command <esc>[1;4;5;7m.
     An argument of 0 clears all attributes, and is the default argument, so we
     use <esc>[m to go back to normal text formatting.
    */

    ab_append(ab, "\x1b[7m", 4);  // invert colors

    // text to write inside statusbar
    char status[80], rstatus[80];
    int status_len = snprintf(status, sizeof(status), "%.20s - %d lines %s",
                              conf->filename ? conf->filename : "[No Name]",
                              conf->numrows, conf->dirty ? "(modified)" : "");

    int rstatus_len = snprintf(rstatus, sizeof(rstatus), "%s | %d/%d",
                               conf->syntax ? conf->syntax->filetype : "no ft",
                               conf->cy + 1, conf->numrows);

    if (status_len > conf->screen_cols) status_len = conf->screen_cols;
    ab_append(ab, status, status_len);

    while (status_len < conf->screen_cols) {
        if (conf->screen_cols - status_len == rstatus_len) {
            ab_append(ab, rstatus, rstatus_len);
            break;
        } else {
            ab_append(ab, " ", 1);
            status_len++;
        }
    }

    ab_append(ab, "\r\n", 2);
    ab_append(ab, "\x1b[m", 3);  // returns to normal

    return 0;
}

int editor_draw_rows(struct Config *conf, struct abuf *ab) {
    for (size_t y = 0; y < (size_t)conf->screen_rows; y++) {
        int filerow = y + conf->rowoff;
        if (filerow >= conf->numrows) {
            // if there are no rows then we can safely assume we need to output
            // welcome screen
            if (conf->numrows == 0 && y == (size_t)conf->screen_rows / 3) {
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

            char *s = &conf->rows[filerow].render[conf->coloff];
            unsigned char *hl = &conf->rows[filerow].hl[conf->coloff];
            int current_color = -1;

            for (size_t j = 0; j < (size_t)len; j++) {
                if (iscntrl(s[j])) {  // TODO
                    char sym = (s[j] <= 26) ? '@' + s[j] : '?';
                    ab_append(ab, "\x1b[7m", 4);
                    ab_append(ab, &sym, 1);
                    ab_append(ab, "\x1b[m", 3);

                    if (current_color != -1) {
                        char buf[16];
                        size_t clen = snprintf(buf, sizeof(buf), "\x1b[%dm",
                                               current_color);
                        ab_append(ab, buf, clen);
                    }

                } else if (hl[j] == HL_NORMAL) {
                    if (current_color != -1) {
                        ab_append(ab, "\x1b[39m", 5);  // white color
                        current_color = -1;
                    }
                    ab_append(ab, &s[j], 1);

                } else {
                    int color = editor_syntax_to_color_row(hl[j]);
                    if (color != current_color) {
                        current_color = color;
                        char buf[16];
                        int clen =
                            snprintf(buf, sizeof(buf), "\x1b[%dm", color);
                        ab_append(ab, buf, clen);
                    }
                    ab_append(ab, &s[j], 1);
                }
            }
            ab_append(ab, "\x1b[39m", 5);  // reset to default color
        }

        ab_append(ab, "\x1b[K", 4);  // erase in line command
        ab_append(ab, "\r\n", 2);
    }

    return 0;
}

/***  Cursor movement and scrolling section ***/

int editor_insert_newline(struct Config *conf) {
    int res;
    if (conf->cx == 0) {
        res = editor_insert_row(conf, conf->cy, "", 0);
    } else {
        struct e_row *row = &conf->rows[conf->cy];
        res = editor_insert_row(conf, conf->cy + 1, &row->chars[conf->cx],
                                row->size - conf->cx);

        row = &conf->rows[conf->cy];  // realloc could make our pointer invalid
                                      // so better safe than sorry
        row->size = conf->cx;
        row->chars[row->size] = '\0';
        editor_update_row(conf, row);
    }
    if (res == 0) {
        conf->cx = 0, conf->cy++;
    }
    return res;
}

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
int editor_cursor_shift(struct Config *conf, enum EditorKey key) {
    struct e_row *row = &conf->rows[conf->cy];

    if (key == CTRL_ARROW_RIGHT) {
        if (conf->cx == (int)row->size) {
            conf->cy++;
            if (conf->cy >= conf->numrows - 1) {
                conf->cy--;
                return 1;
            }
            conf->cx = 0;
            row = &conf->rows[conf->cy];
        }

        while (conf->cx < (int)row->size && !ISCHAR(row->chars[conf->cx]) &&
               (row->chars[conf->cx] != '_'))
            conf->cx++;

        while (conf->cx < (int)row->size &&
               (ISCHAR(row->chars[conf->cx]) || (row->chars[conf->cx] == '_')))
            conf->cx++;

    } else {
        if (conf->cx == 0) {
            conf->cy--;
            if (conf->cy < 0) {
                conf->cy = 0;
                return 1;
            }
            row = &conf->rows[conf->cy];
            conf->cx = row->size != 0 ? row->size - 1 : 0;
        }

        if (conf->cx != 0) {
            conf->cx--;
            while (conf->cx > 0 && !ISCHAR(row->chars[conf->cx]) &&
                   (row->chars[conf->cx] != '_'))
                conf->cx--;

            while (conf->cx > 0 && (ISCHAR(row->chars[conf->cx]) ||
                                    (row->chars[conf->cx] == '_')))
                conf->cx--;

            if (!ISCHAR(row->chars[conf->cx]) &&
                (row->chars[conf->cx] != '_')) {
                conf->cx++;
            }
        }
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
            if (row && conf->cx < (int)row->size) {
                conf->cx++;
            } else if (row && conf->cy < conf->numrows - 1) {
                conf->cx = 0;
                conf->cy++;
            }
            break;
        case ARROW_UP:
            if (conf->cy != 0) conf->cy--;
            break;
        case ARROW_DOWN:
            if (conf->cy < conf->numrows - 1) conf->cy++;
            break;
        default:
            return 1;
    }

    row = (conf->cy >= conf->numrows) ? NULL : &conf->rows[conf->cy];
    if (row && conf->cx > (int)row->size) {
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
        char seq[5];
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
                } else if (seq[2] == ';') {
                    if (read(STDIN_FILENO, &seq[3], 1) != 1) return '\x1b';
                    if (read(STDIN_FILENO, &seq[4], 1) != 1) return '\x1b';
                    if (seq[1] == '1' && seq[3] == '5') {
                        switch (seq[4]) {
                            case 'A':
                                return CTRL_ARROW_UP;
                                break;
                            case 'B':
                                return CTRL_ARROW_DOWN;
                                break;
                            case 'C':
                                return CTRL_ARROW_RIGHT;
                                break;
                            case 'D':
                                return CTRL_ARROW_LEFT;
                                break;
                        }
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

    return 0;
}

int editor_process_key_press(struct Config *conf) {
    /*
                Side Note:
                        We use int c instead of char c since we have mapped
       some keys to values bigger than the max 255 like for ARROWS

    */

    static int quit_times = QUIT_TIMES;

    struct e_row *row =
        (conf->cy >= conf->numrows) ? NULL : &conf->rows[conf->cy];
    int c = editor_read_key();

    int times = conf->screen_rows;  // this will be needed in case of page
                                    // up or down basically
    switch (c) {
        case '\r':
            editor_insert_newline(conf);
            break;

        case ARROW_UP:
        case ARROW_DOWN:
        case ARROW_RIGHT:
        case ARROW_LEFT:
            editor_cursor_move(conf, c);
            break;

        case CTRL_ARROW_UP:
            if (conf->rowoff > 0) {
                conf->cy--;
                conf->rowoff--;
            }
            break;

        case CTRL_ARROW_DOWN:
            if (conf->rowoff < conf->numrows - 6) {
                conf->rowoff++;
                conf->cy++;
            }
            break;

        case CTRL_ARROW_LEFT:
        case CTRL_ARROW_RIGHT:
            editor_cursor_shift(conf, c);
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

        case BACKSPACE:
        case CTRL_KEY('h'):
        case DEL_KEY:
            if (c == DEL_KEY) {
                editor_cursor_move(conf, ARROW_RIGHT);
            }
            editor_delete_char(conf);
            break;

        case CTRL_KEY('q'):
            if (quit_times != 0 && conf->dirty) {
                editor_set_status_message(conf,
                                          "WARNING!!! file has unsaved changes."
                                          "Press CTRL-Q %d time(s) to leave",
                                          quit_times);
                quit_times--;
                return 0;
            }
            if (write(STDOUT_FILENO, "\x1b[2J", 4) == 0) return 1;
            if (write(STDOUT_FILENO, "\x1b[H", 3) == 0) return 1;
            exit(1);
            break;
        case CTRL_KEY('l'):
        case '\x1b':
            // TODO
            break;

        case CTRL_KEY('s'):
            editor_save(conf);
            break;

        case CTRL_KEY('c'):
            editor_copy(conf);
            break;
        case CTRL_KEY('v'):
            editor_paste(conf);
            break;
        case CTRL_KEY('x'):
            editor_cut(conf);
            break;
        case CTRL_KEY('f'):
        case CTRL_KEY('o'):  // TODO: remove me once done with debugging
            editor_find(conf);
            break;
        default:
            editor_insert_char(conf, c);
            break;
    }

    // we reset if user entered something else than a CTRL-Q
    quit_times = QUIT_TIMES;

    return 0;
}