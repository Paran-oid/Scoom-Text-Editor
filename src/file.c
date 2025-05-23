#include "file.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "objects.h"
#include "terminal.h"

int editor_append_row(struct Config* conf, const char* content,
                      size_t content_len) {
    conf->rows =
        realloc(conf->rows, sizeof(struct e_row) * (conf->numrows + 1));
    int at = conf->numrows;

    conf->rows[at].size = content_len;
    conf->rows[at].chars = calloc(content_len + 1, sizeof(char));
    memcpy(conf->rows[at].chars, content, content_len);
    conf->rows[at].chars[content_len] = '\0';
    conf->numrows++;

    return 0;
}

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