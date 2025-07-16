#include "rows.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "core.h"
#include "file.h"
#include "highlight.h"
#include "terminal.h"

int editor_free_row(struct Row* row) {
    free(row->chars);
    free(row->render);
    free(row->hl);
    return EXIT_SUCCESS;
}

int editor_insert_row_char(struct EditorConfig* conf, struct Row* row, int at,
                           int c) {
    if (at < 0 || (size_t)at > row->size) return EXIT_FAILURE;
    // n stands for new for now
    char* new_chars = realloc(row->chars, row->size + 2);
    if (!new_chars) die("new_chars realloc failed");
    row->chars = new_chars;

    memmove(row->chars + at + 1, row->chars + at, row->size - at);
    row->chars[at] = c;

    row->size++;
    row->chars[row->size] = '\0';
    if (editor_update_row(conf, row) == EXIT_FAILURE)
        die("editor update row failed");

    return EXIT_SUCCESS;
}

int editor_delete_row_char(struct EditorConfig* conf, struct Row* row, int at) {
    if (at < 0 || (size_t)at >= row->size) return EXIT_FAILURE;

    memmove(&row->chars[at], &row->chars[at + 1], row->size - at - 1);
    row->size--;

    row->chars = realloc(row->chars, row->size + 1);
    row->chars[row->size] = '\0';

    if (editor_update_row(conf, row) == EXIT_FAILURE)
        die("editor update row failed");
    conf->flags.is_dirty = 1;

    return EXIT_SUCCESS;
}

//* make this code shorter if possible instead of having to do conf->rows[at] do
//* row pointer or something
int editor_insert_row(struct EditorConfig* conf, int at, const char* content,
                      size_t content_len) {
    if (at < 0 || at > conf->numrows) return EXIT_FAILURE;

    conf->rows = realloc(conf->rows, sizeof(struct Row) * (conf->numrows + 1));

    memmove(&conf->rows[at + 1], &conf->rows[at],
            sizeof(struct Row) * (conf->numrows - at));

    for (int j = at + 1; j <= conf->numrows; j++) {
        conf->rows[j].idx++;
    }

    struct Row* row = &conf->rows[at];

    row->idx = at;
    row->size = content_len;
    row->chars = calloc(content_len + 1, sizeof(char));

    // copy spaces if any
    // copy the content
    memcpy(row->chars, content, content_len);

    row->chars[content_len] = '\0';
    row->render = NULL;
    row->rsize = 0;
    row->hl = NULL;
    row->hl_open_comment = 0;

    conf->numrows++;

    conf->flags.is_dirty = 1;
    if (editor_update_row(conf, &conf->rows[at]) == EXIT_FAILURE)
        die("editor update row failed");

    return EXIT_SUCCESS;
}

int editor_update_row(struct EditorConfig* conf, struct Row* row) {
    free(row->render);
    int tabs = 0;
    size_t n = 0;

    // first we need to check how much memory to allocate for the renderer
    for (size_t j = 0; j < row->size; j++) {
        // TODO: make user able to choose between tab and spaces
        if (row->chars[j] == '\t') {
            tabs++;
        }
    }

    row->indentation = tabs;

    /* TAB_SIZE - 1:
        Because the tab is already counted as 1 character in row->size
    */
    row->render = malloc(row->size + tabs * (TAB_SIZE - 1) + 1);
    if (!row->render) die("row render malloc failed");

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

    editor_update_syntax(conf, row);
    return EXIT_SUCCESS;
}

int editor_delete_row(struct EditorConfig* conf, int at) {
    if (at < 0 || at > conf->numrows) return EXIT_FAILURE;
    if (editor_free_row(&conf->rows[at]) == EXIT_FAILURE)
        die("editor free row failed");
    memmove(&conf->rows[at], &conf->rows[at + 1],
            sizeof(struct Row) * (conf->numrows - at - 1));
    for (int j = at; j < conf->numrows - 1; j++) {
        conf->rows[j].idx--;
    }

    if (conf->numrows != 0) conf->numrows--;
    conf->rows = realloc(conf->rows, sizeof(struct Row) * conf->numrows);
    conf->flags.is_dirty = 1;
    return EXIT_SUCCESS;
}

int editor_insert_char(struct EditorConfig* conf, int c) {
    if (conf->cy == conf->numrows)
        editor_insert_row(conf, conf->numrows, "", 0);

    int numline_offset = editor_row_numline_calculate(&conf->rows[conf->cy]);

    conf->flags.is_dirty = 1;
    // got to make sure res is correctly calculated because cx could
    // accidentally touch the numbers part
    int res = editor_insert_row_char(conf, &conf->rows[conf->cy],
                                     conf->cx - numline_offset, c);
    conf->cx++;
    return res;
}

int editor_delete_char(struct EditorConfig* conf) {
    if (conf->numrows == 0) return EXIT_FAILURE;

    int numline_offset = editor_row_numline_calculate(&conf->rows[conf->cy]);

    // make sure we're not at end of file or at beginning of first line
    if (conf->cy == conf->numrows ||
        (conf->cx == numline_offset && conf->cy == 0))
        return EXIT_FAILURE;

    if (conf->cx > numline_offset) {
        int at = conf->cx - numline_offset;
        struct Row* row = &conf->rows[conf->cy];
        char currchar = row->chars[at - 1];
        // handle automated paranthesis removal
        if (check_is_paranthesis(currchar)) {
            char next_char = row->chars[at];
            if (closing_paren(currchar) == next_char) {
                if (editor_delete_row_char(conf, &conf->rows[conf->cy], at) ==
                    EXIT_FAILURE)
                    die("editor delete row char operation failed");
            }
        }

        int res = editor_delete_row_char(conf, &conf->rows[conf->cy], at - 1);
        if (!conf->cx) conf->cx--;

        return res;
    } else {
        conf->cx = conf->rows[conf->cy - 1].size + numline_offset;
        if (editor_row_append_string(conf, &conf->rows[conf->cy - 1],
                                     conf->rows[conf->cy].chars,
                                     conf->rows[conf->cy].size) == EXIT_FAILURE)
            die("editor row append string operation failed");
        if (editor_delete_row(conf, conf->cy) == EXIT_FAILURE)
            die("editor delete row operation failed");
        conf->cy--;
    }
    return EXIT_FAILURE;
}

int editor_rows_to_string(struct EditorConfig* conf, char** result,
                          size_t* result_size) {
    size_t total_size = 0;
    for (size_t i = 0; i < (size_t)conf->numrows; i++) {
        total_size += conf->rows[i].size;

        if (conf->rows[i].size == 0 ||
            conf->rows[i].chars[conf->rows[i].size - 1] != '\n') {
            total_size += 1;
        }
    }

    if (total_size == 0) return EXIT_FAILURE;

    *result_size = total_size;
    char* file_data = malloc(total_size + 1);
    if (!file_data) return -1;

    char* curr_ptr = file_data;

    for (size_t i = 0; i < (size_t)conf->numrows; i++) {
        memcpy(curr_ptr, conf->rows[i].chars, conf->rows[i].size);
        curr_ptr += conf->rows[i].size;

        // Add newline if not present at end of row
        if (conf->rows[i].size == 0 ||
            conf->rows[i].chars[conf->rows[i].size - 1] != '\n') {
            *curr_ptr = '\n';
            curr_ptr++;
        }
    }

    *curr_ptr = '\0';
    *result = file_data;

    return EXIT_SUCCESS;
}

int editor_string_to_rows(struct EditorConfig* conf, char* buffer) {
    if (conf_destroy_rows(conf) == EXIT_FAILURE)
        die("conf destroy rows operation failed");

    char* ptr = buffer;
    size_t index = 0;

    while (*ptr) {
        char* start = ptr;

        while (*ptr != '\n' && *ptr != '\0') ptr++;

        size_t len = ptr - start;
        char* buf = malloc(len + 1);
        if (!buf) die("buf malloc failed");
        memcpy(buf, start, len);
        buf[len] = '\0';

        if (editor_insert_row(conf, index++, buf, len) == EXIT_FAILURE)
            die("editor insert row failed");
        free(buf);

        if (*ptr == '\n') ptr++;
    }

    conf->numrows = index;

    return EXIT_SUCCESS;
}

int editor_row_append_string(struct EditorConfig* conf, struct Row* row,
                             char* s, size_t slen) {
    row->chars = realloc(row->chars, row->size + slen + 1);
    memcpy(&row->chars[row->size], s, slen);
    row->size += slen;
    row->chars[row->size] = '\0';
    editor_update_row(conf, row);
    conf->flags.is_dirty = 1;

    return EXIT_SUCCESS;
}

int editor_update_cx_rx(struct Row* row, int cx) {
    int rx = 0;

    for (size_t j = 0; j < (size_t)cx; j++) {
        if (j < row->size) {
            if (row->chars[j] == '\t') {
                rx += (TAB_SIZE - 1) - (rx % TAB_SIZE);
            }
            rx++;
        }
    }
    return rx;
}

int editor_row_indent(struct EditorConfig* conf, struct Row* row, char** data,
                      size_t* len) {
    int numline_offset = editor_row_numline_calculate(row);
    int indent = row->indentation;  // modified indentation (if needed)

    if (conf->syntax) {
        char language_indent_start = conf->syntax->indent_start;
        char language_indent_end = conf->syntax->indent_end;

        char buf_indent_start[2] = {language_indent_start, '\0'};
        char buf_indent_end[2] = {language_indent_end, '\0'};

        uint8_t in_string = 0;

        // stack will work great to know if an indentation is essential

        Stack* s = malloc(sizeof(Stack));
        stack_create(s, NULL, free);

        for (size_t i = 0; i < (size_t)conf->cx - numline_offset; i++) {
            enum EditorKey c = row->chars[i];

            if (c == '"' && (i == 0 || row->chars[i - 1] != '\\')) {
                in_string = !in_string;
                continue;
            }
            if (!in_string) {
                char* data;
                // handle languages who indent with closing brackets
                if (language_indent_end) {
                    if (c == language_indent_start) {
                        data = strdup(buf_indent_start);
                        stack_push(s, data);
                    } else if (c == language_indent_end) {
                        char* peaked = stack_peek(s);
                        void* ptr;
                        if (peaked && strcmp(peaked, buf_indent_start) == 0) {
                            stack_pop(s, &ptr);
                            free(ptr);
                        } else {
                            data = strdup(buf_indent_end);
                            stack_push(s, data);
                        }
                    }
                } else {
                    if (c == language_indent_start) {
                        data = strdup(buf_indent_start);
                        stack_push(s, data);
                    }
                }
            }
        }

        /*
            If we encounter an open bracket we increase
            identation, else we decrease it
        */

        ListNode* curr = s->head;
        while (curr != NULL) {
            char* data = (char*)curr->data;
            *data == language_indent_start ? indent++ : indent--;
            curr = curr->next;
        }

        stack_destroy(s);
        free(s);
    }

    indent = indent < 0 ? 0 : indent;  // verify it's not less than 0

    int remainder_len = row->size - conf->cx + numline_offset;
    char* remainder = &row->chars[conf->cx - numline_offset];

    char* newline = malloc(remainder_len + indent + 1);
    if (!newline) die("malloc for newline failed");

    memset(newline, '\t', indent);
    memcpy(&newline[indent], remainder, remainder_len);
    newline[remainder_len + indent] = '\0';

    *data = newline;
    *len = remainder_len + indent;

    return EXIT_SUCCESS;
}

int editor_update_rx_cx(struct Row* row, int rx) {
    int curr_rx = 0;
    int cx = 0;

    for (; cx < (int)row->size; cx++) {
        if (row->chars[cx] == '\t') {
            curr_rx += (TAB_SIZE - 1) - (curr_rx % TAB_SIZE);
        }

        curr_rx++;

        if (curr_rx > rx) return cx;
    }

    return cx;
}

// numline has a variable length so we need a respective function for it
int editor_row_numline_calculate(const struct Row* row) {
    return count_digits(row->idx + 1) + 1;
}
