#include "operations.h"

#include <stdlib.h>
#include <string.h>

#include "file.h"
#include "objects.h"
#include "terminal.h"

int editor_free_row(struct e_row* row) {
    free(row->chars);
    free(row->render);
}

int editor_insert_row_char(struct e_row* row, int at, int c) {
    if (at < 0 || at > row->size) {
        return 1;
    }
    // n stands for new for now
    row->chars = realloc(row->chars, row->size + 1);
    if (!row->chars) die("realloc failed");

    memmove(row->chars + at + 1, row->chars + at, row->size - at);
    row->chars[at] = c;

    row->size++;
    editor_update_row(row);

    return 0;
}

int editor_delete_row_char(struct Config* conf, struct e_row* row, int at) {
    if (at < 0 || at > row->size) return 1;

    memmove(&row->chars[at], &row->chars[at + 1], row->size - at);
    row->size--;
    editor_update_row(row);
    conf->dirty = 1;
}

int editor_append_row(struct Config* conf, const char* content,
                      size_t content_len) {
    conf->rows =
        realloc(conf->rows, sizeof(struct e_row) * (conf->numrows + 1));
    int at = conf->numrows;

    conf->rows[at].size = content_len;
    conf->rows[at].chars = calloc(content_len + 1, sizeof(char));
    memcpy(conf->rows[at].chars, content, content_len);
    conf->rows[at].chars[content_len] = '\0';

    conf->rows[at].render = NULL;
    conf->rows[at].rsize = 0;

    conf->numrows++;

    conf->dirty = 1;
    editor_update_row(&conf->rows[at]);

    return 0;
}

int editor_update_row(struct e_row* row) {
    free(row->render);

    // we need to check how much memory to allocate for the renderer
    int tabs = 0;
    size_t n = 0;
    for (size_t j = 0; j < row->size; j++) {
        if (row->chars[j] == '\t') tabs++;
    }

    /* TAB_SIZE - 1:
        Because the tab is already counted as 1 character in row->size
    */
    row->render = malloc(row->size + tabs * (TAB_SIZE - 1) + 1);

    for (size_t j = 0; j < row->size; j++) {
        /*
                If a tab is encountered:
                - keep adding spaces until n is divisible by TAB_SIZE macro
        */
        if (row->chars[j] == '\t') {
            row->render[n++] = ' ';
            while (n % TAB_SIZE != 0) {
                row->render[n++] = ' ';
            }
        } else {
            row->render[n++] = row->chars[j];
        }
    }

    row->rsize = n;
    row->render[n] = '\0';
    return 0;
}

int editor_delete_row(struct Config* conf, int at) {
    if (at < 0 || at > conf->numrows) return 1;
    editor_free_row(&conf->rows[at]);
    memmove(&conf->rows[at], &conf->rows[at + 1],
            sizeof(struct e_row) * (conf->numrows - at - 1));
    conf->numrows--;
    conf->dirty = 1;
    return 0;
}

int editor_insert_char(struct Config* conf, int c) {
    if (conf->cy == conf->numrows) editor_append_row(conf, "", 0);
    conf->dirty = 1;
    return editor_insert_row_char(&conf->rows[conf->cy], conf->cx++, c);
}

int editor_delete_char(struct Config* conf) {
    if (conf->cy == conf->numrows) return 1;
    if (conf->cx > 0) {
        return editor_delete_row_char(conf, &conf->rows[conf->cy], --conf->cx);
    } else {
        conf->cx = conf->rows[conf->cy - 1].size;
        editor_row_append_string(conf, &conf->rows[conf->cy - 1],
                                 conf->rows[conf->cy].chars,
                                 conf->rows[conf->cy].size);
        editor_delete_row(conf, conf->cy);
        conf->cy--;
    }
    return 2;
}

int editor_rows_to_string(struct Config* conf, char** result,
                          size_t* result_size) {
    size_t total_size = 0;
    for (size_t i = 0; i < conf->numrows; i++) {
        total_size += conf->rows[i].size + 1;
    }
    *result_size = total_size;
    char* file_data = malloc(total_size);
    if (!file_data) return -1;

    char* curr_ptr = file_data;

    for (size_t i = 0; i < conf->numrows; i++) {
        memcpy(curr_ptr, conf->rows[i].chars, conf->rows[i].size);
        curr_ptr += conf->rows[i].size;
        *curr_ptr = '\n';
        curr_ptr++;
    }

    *result = file_data;

    return 0;
}

int editor_row_append_string(struct Config* conf, struct e_row* row, char* s,
                             size_t slen) {
    row->chars = realloc(row->chars, row->size + slen + 1);
    memcpy(&row->chars[row->size], s, slen);
    row->size += slen;
    row->chars[row->size] = '\0';
    editor_update_row(row);
    conf->dirty = 1;
}

int editor_update_cx_rx(struct e_row* row, int cx) {
    int rx = 0;
    for (size_t j = 0; j < cx; j++) {
        if (row->chars[j] == '\t') {
            rx += (TAB_SIZE - 1) - (rx % TAB_SIZE);
        }
        rx++;
    }
    return rx;
}
