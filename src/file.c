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

int snapshot_create(struct EditorConfig* conf, struct Snapshot* snapshot) {
    snapshot->cx = conf->cx;
    snapshot->cy = conf->cy;
    snapshot->numrows = conf->numrows;

    editor_rows_to_string(conf, &snapshot->text, &snapshot->len);

    return SUCCESS;
}

int snapshot_destroy(struct Snapshot* snapshot) {
    if (snapshot->text) free(snapshot->text);
    return SUCCESS;
}

int editor_open(struct EditorConfig* conf, const char* path) {
    free(conf->filepath);
    conf->filepath = strdup(path);
    if (!conf->filepath) return OUT_OF_MEMORY;

    editor_syntax_highlight_select(conf);

    FILE* fp = fopen(path, "r");
    if (!fp) {
        // create the file if it doesn't exist

        fp = fopen(path, "w");
        if (!fp) die("fopen");

        conf->is_dirty = 0;
        return SUCCESS;
    };

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

    conf->is_dirty = 0;
    free(line);
    fclose(fp);

    return SUCCESS;
}
int editor_run(struct EditorConfig* conf) {
    conf_create(conf);
    term_create(conf);

#ifdef DEBUG
    if (editor_open(conf, "test.c") != SUCCESS) {
        conf_destroy(conf);
        return FILE_OPEN_FAILED;
    }

#endif

    editor_set_status_message(
        conf, "HELP: CTRL-S = save | CTRL-Q = Quit | CTRL-F = Find");

    while (1) {
        editor_refresh_screen(conf);
        if (editor_process_key_press(conf) == EXIT_CODE) {
            break;
        }
    }

    if (write(STDOUT_FILENO, "\x1b[2J", 4) == 0) return FILE_WRITE_FAILED;
    if (write(STDOUT_FILENO, "\x1b[H", 3) == 0) return FILE_WRITE_FAILED;
    return SUCCESS;
}

int editor_extract_filename(struct EditorConfig* conf, char** filename) {
    if (!conf->filepath) return ERROR;
    char* file;
    int count_slash = count_char(conf->filepath, strlen(conf->filepath), '/');
    if (count_slash) {
        file = strrchr(conf->filepath, '/');
    } else {
        file = conf->filepath;
    }

    *filename = file;

    return SUCCESS;
}

int editor_destroy(struct EditorConfig* conf) {
    conf_destroy(conf);
    return SUCCESS;
}

// TODO: generates weird character at end of file each time TO FIX
int editor_save(struct EditorConfig* conf) {
    if (!conf->filepath) {
        conf->filepath = editor_prompt(conf, "Save as: %s", NULL);
        if (!conf->filepath) {
            editor_set_status_message(conf, "Save aborted...");
            return 1;
        }
    }

    editor_syntax_highlight_select(conf);

    char* file_data;
    size_t file_data_size;

    editor_rows_to_string(conf, &file_data, &file_data_size);

    int fd = open(conf->filepath, O_CREAT | O_WRONLY, 0644);

    if (!fd) return -1;
    // update size of file to one inserted
    if (ftruncate(fd, file_data_size) == -1) return 1;
    if ((size_t)write(fd, file_data, file_data_size) != file_data_size)
        return 1;

    if (file_data_size) {
        editor_set_status_message(conf, "%d bytes written to disk",
                                  file_data_size);
    } else {
        editor_set_status_message(conf, "saved empty file", file_data_size);
    }

    conf->is_dirty = 0;
    close(fd);
    free(file_data);

    return SUCCESS;
}

int editor_undo(struct EditorConfig* conf) {
    if (stack_size(conf->stack_undo) == 0) return UNDO_STACK_EMPTY;

    struct Snapshot *popped_snapshot, *current_snapshot;

    current_snapshot = malloc(sizeof(struct Snapshot));
    snapshot_create(conf, current_snapshot);
    stack_push(conf->stack_redo, current_snapshot);

    stack_pop(conf->stack_undo, (void**)&popped_snapshot);
    conf_to_snapshot_update(conf, popped_snapshot);
    free(popped_snapshot);

    editor_set_status_message(conf, "undo success!");

    return SUCCESS;
}

int editor_redo(struct EditorConfig* conf) {
    if (stack_size(conf->stack_redo) == 0) return REDO_STACK_EMPTY;

    struct Snapshot *popped_snapshot, *current_snapshot;

    current_snapshot = malloc(sizeof(struct Snapshot));
    snapshot_create(conf, current_snapshot);
    stack_push(conf->stack_undo, current_snapshot);

    stack_pop(conf->stack_redo, (void**)&popped_snapshot);
    conf_to_snapshot_update(conf, popped_snapshot);

    editor_set_status_message(conf, "redo success!");

    return SUCCESS;
}

int editor_copy(struct EditorConfig* conf) {
    /*
            Sadly just linux compatible at the moment...
    */

    FILE* pipe = popen("xclip -selection clipboard", "w");
    if (!pipe) return 1;

    struct Row* row = &conf->rows[conf->cy];
    if (fwrite(row->chars, sizeof(char), row->size, pipe) == 0) {
        pclose(pipe);
        return 2;
    }

    editor_set_status_message(conf, "copied %d bytes into buffer", row->size);
    pclose(pipe);
    return SUCCESS;
}

// TODO: handle edge case of empty file
// TODO: document this very important
static size_t editor_paste_buffer(struct EditorConfig* conf,
                                  char** buffer_content, size_t n) {
    if (n == 0) {
        return -1;
    }

    /*
            We do -1 for all buffer_content char* because it has \n at end
    */

    size_t total_size = 0;

    struct Row* row = &conf->rows[conf->cy];
    int numline_size = editor_row_numline_calculate(row);
    int cursor_offset = conf->cx - numline_size;

    char* first_buffer_content = buffer_content[0];
    size_t first_buffer_size = strlen(first_buffer_content) - 1;

    char* str_append = malloc(cursor_offset + first_buffer_size + 1);
    char* str_remaining = strdup(row->chars + cursor_offset);
    size_t str_remaining_size = strlen(str_remaining);

    if (!str_remaining) {
        free(str_append);
        return -1;  // TODO: make this an enum type
    }

    memcpy(str_append, row->chars, cursor_offset);
    memcpy(str_append + cursor_offset, first_buffer_content, first_buffer_size);
    str_append[cursor_offset + first_buffer_size] = '\0';

    total_size += first_buffer_size;

    editor_delete_row(conf, conf->cy);
    editor_insert_row(conf, conf->cy, str_append,
                      first_buffer_size + cursor_offset);

    // add each independent buffer_row if in [1, n - 2]
    if (n > 1) {
        for (size_t i = 1; i < n - 2; i++) {
            conf->cy++;
            editor_insert_row(conf, conf->cy, buffer_content[i],
                              strlen(buffer_content[i]));
            total_size += strlen(buffer_content[i]);
        }
    }

    // append remaining
    row = &conf->rows[conf->cy];
    char* last_buffer_content = buffer_content[n - 1];
    size_t last_row_size = strlen(last_buffer_content);

    // TODO: instead of manually appending, make a function in core.h/.c for it!
    char* last_modified_row = malloc(last_row_size + str_remaining_size + 1);
    memcpy(last_modified_row, last_row, last_row_size);
    memcpy(last_modified_row + last_row_size, str_remaining,
           str_remaining_size);
    last_modified_row[last_row_size + str_remaining_size] = '\0';

    editor_insert_row(conf, conf->cy, last_modified_row,
                      strlen(last_modified_row));

    total_size += strlen(last_row);
    conf->cx = last_row_size;

    free(last_modified_row);
    free(str_append);
    free(str_remaining);
    return total_size;
}

// TODO: make it cross platform maybe
int editor_paste(struct EditorConfig* conf) {
    FILE* pipe = popen("xclip -selection clipboard -o", "r");
    if (!pipe) return ERROR;

    char** buffer_content = NULL;
    size_t n = 0;

    char* content_pasted = NULL;
    size_t size = 0;
    ssize_t len = 0;

    while ((len = getline(&content_pasted, &size, pipe)) != -1) {
        buffer_content = realloc(buffer_content, sizeof(char*) * (n + 1));
        buffer_content[n] = strdup(content_pasted);
        if (!buffer_content || !buffer_content[n]) {
            for (size_t i = 0; i < n; i++) {
                free(buffer_content[i]);
            }
            free(buffer_content);
        }
        n++;
    }

    pclose(pipe);

    if (n == 1 && len == -1 && *content_pasted == '\0') {
        return EMPTY_COPY_BUFFER;
    }

    /*
The code compiled without errors
The code compiled without errors.
The code compiled without errors.
The code compiled without errors.
     */

    if (conf->cy == conf->numrows) {
        editor_insert_row(conf, conf->cy, "", 0);
    }

    size_t total_bytes_pasted = editor_paste_buffer(conf, buffer_content, n);

    if (total_bytes_pasted > -1) {
        editor_set_status_message(conf, "pasted %d bytes into buffer",
                                  total_bytes_pasted);
    } else {
        editor_set_status_message(conf, "error encountered while pasting...");
    }

    free(content_pasted);

    return SUCCESS;
}
int editor_cut(struct EditorConfig* conf) {
    FILE* pipe = popen("xclip -selection clipboard", "w");
    if (!pipe) return 1;

    struct Row* row = &conf->rows[conf->cy];
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
    return SUCCESS;
}

static void editor_find_callback(struct EditorConfig* conf, char* query,
                                 int key) {
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
        // bring back to old snapshot then return
        last_match = -1;
        direction = 1;
        return;
    } else if (key == ARROW_RIGHT || key == ARROW_DOWN) {
        direction = 1;
    } else if (key == ARROW_LEFT || key == ARROW_UP) {
        direction = -1;
    } else {
        // if none just bring back to old snapshot
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

        struct Row* row = &conf->rows[current];
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

int editor_find(struct EditorConfig* conf) {
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

    return SUCCESS;
}