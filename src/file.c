#include "file.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "config.h"
#include "core.h"
#include "highlight.h"
#include "input.h"
#include "render.h"
#include "rows.h"
#include "terminal.h"

int editor_open(struct Config* conf, const char* path) {
    free(conf->filename);
    conf->filename = strdup(path);

    editor_syntax_highlight_select(conf);

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
        conf->filename = editor_prompt(conf, "Save as: %s", NULL);
        if (!conf->filename) {
            editor_set_status_message(conf, "Save aborted...");
            return 1;
        }
    }

    editor_syntax_highlight_select(conf);

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

    if (conf->cy == conf->numrows) {
        editor_insert_row(conf, conf->cy, "", 0);
    }
    struct e_row* row = &conf->rows[conf->cy];
    char* new_chars = malloc(row->size + len + 1);
    int numline_size = editor_row_numline_calculate(row);

    int before = conf->cx - numline_size;

    strncpy(new_chars, row->chars, before);            // copy before cursor
    strncpy(new_chars + before, content_pasted, len);  // copy content pasted
    strncpy(new_chars + before + len, row->chars + before,
            row->size - before);  // copy rest of the string
    new_chars[row->size + len] = '\0';

    free(row->chars);
    row->chars = new_chars;
    row->size = row->size + len;
    editor_update_row(conf, row);
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

    size_t rowsize = row->size;

    if (conf->cy == 0) {
        if (conf->rows[conf->cy].size == 0) {
            return 1;
        } else {
            editor_delete_row(conf, 0);
            editor_insert_row(conf, 0, "", 0);
            conf->cx = 0;
        }
    };

    editor_set_status_message(conf, "cut %d bytes into buffer", rowsize);

    if (conf->cy != 0) {
        editor_delete_row(conf, (intptr_t)(row - conf->rows));
        conf->cy--;
    }

    pclose(pipe);
    return 0;
}

static void editor_find_callback(struct Config* conf, char* query, int key) {
    // direction: 1(forward) / -1(backward)
    // last_match: -1(not found) / 1(found)

    static int last_match = -1;
    static int direction = 1;

    static int saved_hl_line;
    static char* saved_hl = NULL;

    if (saved_hl) {
        memcpy(conf->rows[saved_hl_line].hl, saved_hl,
               conf->rows[saved_hl_line].rsize);
        free(saved_hl);
        saved_hl = NULL;
    }

    if (key == '\r' || key == '\x1b') {
        // bring back to old state then return
        last_match = -1;
        direction = 1;
        return;
    } else if (key == ARROW_RIGHT || key == ARROW_DOWN) {
        direction = 1;
    } else if (key == ARROW_LEFT || key == ARROW_UP) {
        direction = -1;
    } else {
        // if none just bring back to old state
        last_match = -1;
        direction = 1;
    }

    if (last_match == -1) direction = 1;
    // index of current row
    int current = last_match;

    for (size_t i = 0; i < (size_t)conf->numrows; i++) {
        current += direction;
        if (current < 0)
            current = conf->numrows - 1;
        else if (current >= conf->numrows)
            current = 0;
        // if user tried to go back before the first occurred element

        struct e_row* row = &conf->rows[current];
        char* match = strstr(row->render, query);
        if (match) {
            last_match = current;
            conf->cy = current;
            conf->cx = editor_update_rx_cx(row, match - row->render);
            conf->rowoff = conf->cy;

            saved_hl_line = current;
            saved_hl = malloc(row->rsize);
            memcpy(saved_hl, row->hl, row->rsize);
            memset(&row->hl[match - row->render], HL_MATCH, strlen(query));

            break;
        }
    }
}

int editor_find(struct Config* conf) {
    int saved_cx = conf->cx, saved_cy = conf->cy, saved_rowoff = conf->rowoff,
        saved_coloff = conf->coloff;

    char* query = editor_prompt(conf, "Search: %s (use ESC/Arrow/Enter)",
                                editor_find_callback);

    if (query) {
        free(query);
    } else {
        conf->cx = saved_cx;
        conf->cy = saved_cy;
        conf->rowoff = saved_rowoff;
        conf->coloff = saved_coloff;
    }

    return 0;
}