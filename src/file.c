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

int editor_open(struct Config* conf, const char* path) {
    free(conf->filename);
    conf->filename = strdup(path);

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
    if (write(fd, file_data, file_data_size) != file_data_size) return 1;

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
    if (fwrite(row->chars, sizeof(char), row->size, pipe) < 0) {
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

    char* buf;
    size_t size = 0;
    ssize_t len = getline(&buf, &size, pipe);

    if (len == -1) {
        free(buf);
        return 2;
    }

    pclose(pipe);

    struct e_row* row = &conf->rows[conf->cy];
    char* new_chars = malloc(row->size + len + 1);

    strncpy(new_chars, row->chars, conf->cx);
    strncpy(new_chars + conf->cx, buf, len);
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

    return 0;
}
int editor_cut(struct Config* conf) {
    FILE* pipe = popen("xclip -selection clipboard", "w");
    if (!pipe) return 1;

    struct e_row* row = &conf->rows[conf->cy];
    if (fwrite(row->chars, sizeof(char), row->size, pipe) < 0) {
        pclose(pipe);
        return 2;
    }

    editor_set_status_message(conf, "cut %d bytes into buffer", row->size);

    editor_delete_row(conf, (intptr_t)(row - conf->rows));
    pclose(pipe);
    return 0;
}