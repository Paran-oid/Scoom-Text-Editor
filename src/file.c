#include "file.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "core.h"
#include "inout.h"
#include "objects.h"
#include "operations.h"
#include "terminal.h"

int editor_create(struct Config* conf) {
    conf->filename = NULL;

    // status message section
    conf->sbuf[0] = '\0';
    conf->sbuf_time = 0;

    // cursor section
    conf->cx = 0;
    conf->cy = 0;
    conf->rx = 0;

    conf->numrows = 0;
    conf->rows = NULL;
    conf->rowoff = 0;
    conf->coloff = 0;
    conf->dirty = 0;
    if (term_get_window_size(conf, &conf->screen_rows, &conf->screen_cols) !=
        0) {
        return -1;
    }

    conf->screen_rows -= 2;

    return 0;
}

int editor_destroy(struct Config* conf) {
    if (conf->filename) free(conf->filename);

    for (size_t i = 0; i < (size_t)conf->numrows; i++) {
        free(conf->rows[i].chars);
        free(conf->rows[i].render);
    }

    free(conf->rows);
    free(conf);

    return 0;
}

int editor_open(struct Config* conf, const char* path) {
    free(conf->filename);
    conf->filename = strdup(path);

    FILE* fp = fopen(path, "r");
    if (!fp) die("fopen");

    char* line = NULL;
    size_t line_cap = 0;  // size of buf basically
    ssize_t line_len = 0;
    while ((line_len = getline(&line, &line_cap, fp)) != -1) {
        // gotta make sure there ain't any \r or \n mfs out there
        while (line_len > 0 &&
               (line[line_len - 1] == '\r' || line[line_len - 1] == '\n')) {
            line_len--;
        }
        editor_insert_row(conf, conf->numrows, line, line_len);
    }

    conf->dirty = 0;
    free(line);
    fclose(fp);

    return 0;
}

int editor_save(struct Config* conf) {
    if (!conf->filename) {
        conf->filename = editor_prompt(conf, "Save as: %s");
        if (!conf->filename) {
            editor_set_status_message(conf, "Save aborted...");
            return 1;
        }
    }

    char* file_data;
    size_t file_data_size;

    editor_rows_to_string(conf, &file_data, &file_data_size);
    int fd = open(conf->filename, O_CREAT | O_WRONLY, 0644);

    if (!fd) return -1;
    // update size of file to one inserted
    if (ftruncate(fd, file_data_size) == -1) return 1;
    if ((size_t)write(fd, file_data, file_data_size) != file_data_size)
        return 1;

    editor_set_status_message(conf, "%d bytes written to disk", file_data_size);

    conf->dirty = 0;
    close(fd);
    free(file_data);

    return 0;
}

int editor_copy(struct Config* conf) {
    /*
            Sadly just linux compatible at the moment...
    */

    FILE* pipe = popen("xclip -selection clipboard", "w");
    if (!pipe) return 1;

    struct e_row* row = &conf->rows[conf->cy];
    if (fwrite(row->chars, sizeof(char), row->size, pipe) == 0) {
        pclose(pipe);
        return 2;
    }

    editor_set_status_message(conf, "copied %d bytes into buffer", row->size);
    pclose(pipe);
    return 0;
}

int editor_paste(struct Config* conf) {
    FILE* pipe = popen("xclip -selection clipboard -o", "r");
    if (!pipe) return 1;

    char* content_pasted;
    size_t size = 0;
    ssize_t len = getline(&content_pasted, &size, pipe);

    if (len == -1) {
        free(content_pasted);
        return 2;
    }

    pclose(pipe);

    struct e_row* row = &conf->rows[conf->cy];
    char* new_chars = malloc(row->size + len + 1);

    strncpy(new_chars, row->chars, conf->cx);
    strncpy(new_chars + conf->cx, content_pasted, len);
    strncpy(new_chars + conf->cx + len, row->chars + conf->cx,
            row->size - conf->cx);
    new_chars[row->size + len] = '\0';

    free(row->chars);
    row->chars = new_chars;
    row->size = row->size + len;
    editor_update_row(row);
    conf->dirty = 1;

    editor_set_status_message(conf, "pasted %d bytes into buffer",
                              row->size + len + 1);

    conf->cx += len;

    free(content_pasted);

    return 0;
}
int editor_cut(struct Config* conf) {
    FILE* pipe = popen("xclip -selection clipboard", "w");
    if (!pipe) return 1;

    struct e_row* row = &conf->rows[conf->cy];
    if (fwrite(row->chars, sizeof(char), row->size, pipe) == 0) {
        pclose(pipe);
        return 2;
    }

    editor_set_status_message(conf, "cut %d bytes into buffer", row->size);

    editor_delete_row(conf, (intptr_t)(row - conf->rows));
    pclose(pipe);
    return 0;
}

int editor_find(struct Config* conf) {
    char* query = editor_prompt(conf, "Search: %s (ESC to cancel)");
    if (!query) return 1;

    for (size_t i = 0; i < (size_t)conf->numrows; i++) {
        struct e_row* row = &conf->rows[i];
        char* match = strstr(row->render, query);
        if (match) {
            conf->cx = editor_update_rx_cx(row, match - row->render);
            conf->cy = i;
            conf->rowoff = conf->cy;
            break;
        }
    }

    free(query);

    return 0;
}