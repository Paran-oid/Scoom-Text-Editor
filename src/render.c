#include "render.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "buffer.h"
#include "config.h"
#include "core.h"
#include "file.h"
#include "highlight.h"
#include "input.h"
#include "rows.h"
#include "terminal.h"
/***  Appending buffer section ***/

char *editor_prompt(struct EditorConfig *conf, const char *prompt,
                    void (*callback)(struct EditorConfig *, char *, int)) {
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

int editor_set_status_message(struct EditorConfig *conf, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(conf->status_msg, sizeof(conf->status_msg), fmt, ap);
    va_end(ap);
    conf->sbuf_time = time(NULL);

    return EXIT_SUCCESS;
}

/***  Screen display and rendering section ***/

int editor_refresh_screen(struct EditorConfig *conf) {
    editor_scroll(conf);

    struct ABuf ab = ABUF_INIT;
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
        return EXIT_FAILURE;
    }

    ab_free(&ab);
    return EXIT_SUCCESS;
}

int editor_draw_messagebar(struct EditorConfig *conf, struct ABuf *ab) {
    ab_append(ab, "\x1b[K", 3);  // we clear current line in terminal
    size_t message_len = strlen(conf->status_msg);
    if (message_len > (size_t)conf->screen_cols)
        message_len = conf->screen_cols;
    // if message has length and time elapsed since
    // last time message inserted is bigger than 5
    if (message_len && time(NULL) - conf->sbuf_time < 5)
        ab_append(ab, conf->status_msg, message_len);

    return EXIT_SUCCESS;
}

int editor_draw_statusbar(struct EditorConfig *conf, struct ABuf *ab) {
    /*
    you could specify all of these attributes using the command <esc>[1;4;5;7m.
    An argument of 0 clears all attributes, and is the default argument, so we
    use <esc>[m to go back to normal text formatting.
    */

    ab_append(ab, "\x1b[7m", 4);  // invert colors

    // text to write inside statusbar
    char status[80], rstatus[80];
    int status_len =
        snprintf(status, sizeof(status), "%.20s - %d lines %s",
                 conf->filename ? conf->filename : "[No Name]", conf->numrows,
                 conf->is_dirty ? "(modified)" : "");

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

    return EXIT_SUCCESS;
}

int editor_draw_rows(struct EditorConfig *conf, struct ABuf *ab) {
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
            // numline section
            int filerow_num = conf->rows[filerow].idx + 1;
            char offset[16];
            int offset_size =
                snprintf(offset, sizeof(offset), "%d ", filerow_num);

            int rowlen = conf->rows[filerow].rsize - conf->coloff;
            if (rowlen < 0) rowlen = 0;
            if (rowlen > conf->screen_cols - offset_size) {
                rowlen = conf->screen_cols - offset_size;
            }

            // appending numline to buff

            char *s = calloc(offset_size + rowlen, sizeof(char));
            memcpy(s, offset, offset_size);
            memcpy(s + offset_size, &conf->rows[filerow].render[conf->coloff],
                   rowlen);

            // highlighting and control section
            unsigned char *hl = &conf->rows[filerow].hl[conf->coloff];
            int current_color = -1;

            for (int j = 0; j < rowlen + offset_size; j++) {
                if (j < offset_size) {
                    ab_append(ab, &s[j], 1);
                    continue;
                }

                if (iscntrl(s[j])) {
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

                } else if (hl[j - offset_size] == HL_NORMAL) {
                    if (current_color != -1) {
                        ab_append(ab, "\x1b[39m", 5);  // white color
                        current_color = -1;
                    }
                    ab_append(ab, &s[j], 1);

                } else {
                    int color = editor_syntax_to_color_row(hl[j - offset_size]);
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
            free(s);
            ab_append(ab, "\x1b[39m", 5);  // reset to default color
        }
        ab_append(ab, "\x1b[K", 4);  // erase in line command
        ab_append(ab, "\r\n", 2);
    }

    return EXIT_SUCCESS;
}

int editor_scroll(struct EditorConfig *conf) {
    struct Row *row;
    row = &conf->rows[conf->cy];

    int numline_size = editor_row_numline_calculate(row);
    conf->rx = conf->cx;

    if (conf->cy < conf->numrows) {
        conf->rx =
            editor_update_cx_rx(&conf->rows[conf->cy], conf->cx - numline_size);
        conf->rx += numline_size;  // I don't know what I am doing I am sorry 😭
    }

    if (conf->rx < conf->coloff + numline_size) {
        conf->coloff = conf->rx - numline_size;
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
    return EXIT_SUCCESS;
}