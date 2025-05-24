#include "file.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "objects.h"
#include "terminal.h"

int editor_open(struct Config* conf, const char* path) {
    FILE* fp = fopen(path, "r");
    if (!fp) die("fopen");

    char* line = NULL;
    size_t line_cap = 0;  // size of buf basically
    size_t line_len = 0;
    while ((line_len = getline(&line, &line_cap, fp)) != -1) {
        // gotta make sure there ain't any \r or \n mfs out there
        while (line_len > 0 &&
               (line[line_len - 1] == '\r' || line[line_len - 1] == '\n')) {
            line_len--;
        }
        editor_append_row(conf, line, line_len);
    }

    free(line);
    fclose(fp);

    return 0;
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

    editor_update_row(&conf->rows[at]);

    return 0;
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