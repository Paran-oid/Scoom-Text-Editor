#include "file.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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
