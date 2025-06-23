#include "rows.h"

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
    return SUCCESS;
}

int editor_insert_row_char(struct EditorConfig* conf, struct Row* row, int at,
                           int c) {
    if (at < 0 || (size_t)at > row->size) {
        return CURSOR_OUT_OF_BOUNDS;
    }
    // n stands for new for now
    char* new_chars = realloc(row->chars, row->size + 1);
    if (!new_chars) return OUT_OF_MEMORY;
    row->chars = new_chars;

    memmove(row->chars + at + 1, row->chars + at, row->size - at);
    row->chars[at] = c;

    row->size++;
    editor_update_row(conf, row);

    return SUCCESS;
}

int editor_delete_row_char(struct EditorConfig* conf, struct Row* row, int at) {
    if (at < 0 || (size_t)at > row->size) return CURSOR_OUT_OF_BOUNDS;

    memmove(&row->chars[at], &row->chars[at + 1], row->size - at - 1);
    row->size--;
    editor_update_row(conf, row);
    conf->is_dirty = 1;

    return SUCCESS;
}

//* make this code shorter if possible instead of having to do conf->rows[at] do
//* row pointer or something
int editor_insert_row(struct EditorConfig* conf, int at, const char* content,
                      size_t content_len) {
    if (at < 0 || at > conf->numrows) return CURSOR_OUT_OF_BOUNDS;

    conf->rows = realloc(conf->rows, sizeof(struct Row) * (conf->numrows + 1));

    memmove(&conf->rows[at + 1], &conf->rows[at],
            sizeof(struct Row) * (conf->numrows - at));

    for (int j = at + 1; j <= conf->numrows; j++) {
        conf->rows[j].idx++;
    }

    conf->rows[at].idx = at;
    conf->rows[at].size = content_len;
    conf->rows[at].chars = calloc(content_len + 1, sizeof(char));

    // copy spaces if any
    // copy the content
    memcpy(conf->rows[at].chars, content, content_len);

    conf->rows[at].chars[content_len] = '\0';
    conf->rows[at].render = NULL;
    conf->rows[at].rsize = 0;
    conf->rows[at].hl = NULL;
    conf->rows[at].hl_open_comment = false;

    conf->numrows++;

    conf->is_dirty = 1;
    editor_update_row(conf, &conf->rows[at]);

    return SUCCESS;
}
// TODO: once created this function use it in other needed programs.
int editor_modify_row(struct EditorConfig* conf, struct Row* row) {
    (void)row;
    (void)conf;
    return SUCCESS;
}

int editor_update_row(struct EditorConfig* conf, struct Row* row) {
    free(row->render);
    // we need to check how much memory to allocate for the renderer
    int tabs = 0;
    size_t n = 0;
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
    return SUCCESS;
}

int editor_delete_row(struct EditorConfig* conf, int at) {
    if (at < 0 || at > conf->numrows) return CURSOR_OUT_OF_BOUNDS;
    editor_free_row(&conf->rows[at]);
    memmove(&conf->rows[at], &conf->rows[at + 1],
            sizeof(struct Row) * (conf->numrows - at - 1));
    for (int j = at; j < conf->numrows - 1; j++) {
        conf->rows[j].idx--;
    }
    conf->numrows--;
    conf->is_dirty = 1;
    return SUCCESS;
}

int editor_insert_char(struct EditorConfig* conf, int c) {
    if (conf->cy == conf->numrows)
        editor_insert_row(conf, conf->numrows, "", 0);

    int numline_offset_size =
        editor_row_numline_calculate(&conf->rows[conf->cy]);

    conf->is_dirty = 1;
    // got to make sure res is correctly calculated because cx could
    // accidentally touch the numbers part
    int res = editor_insert_row_char(conf, &conf->rows[conf->cy],
                                     conf->cx - numline_offset_size, c);
    conf->cx++;
    return res;
}

int editor_delete_char(struct EditorConfig* conf) {
    int numline_offset_size =
        editor_row_numline_calculate(&conf->rows[conf->cy]);

    // make sure we're not at end of file or at beginning of first line
    if (conf->cy == conf->numrows ||
        (conf->cx == numline_offset_size && conf->cy == 0))
        return CURSOR_OUT_OF_BOUNDS;

    if (conf->cx > numline_offset_size) {
        int at = conf->cx - numline_offset_size;
        struct Row* row = &conf->rows[conf->cy];
        char currchar = row->chars[at - 1];
        // handle automated paranthesis removal
        if (check_is_paranthesis(currchar)) {
            char next_char = row->chars[at];
            if (closing_paren(currchar) == next_char) {
                editor_delete_row_char(conf, &conf->rows[conf->cy], at);
            }
        }

        int res = editor_delete_row_char(conf, &conf->rows[conf->cy], at - 1);
        conf->cx--;
        return res;
    } else {
        conf->cx = conf->rows[conf->cy - 1].size + numline_offset_size;
        editor_row_append_string(conf, &conf->rows[conf->cy - 1],
                                 conf->rows[conf->cy].chars,
                                 conf->rows[conf->cy].size);
        editor_delete_row(conf, conf->cy);
        conf->cy--;
    }
    return ERROR;
}

int editor_rows_to_string(struct EditorConfig* conf, char** result,
                          size_t* result_size) {
    size_t total_size = 0;
    for (size_t i = 0; i < (size_t)conf->numrows; i++) {
        total_size += conf->rows[i].size + 1;
    }
    *result_size = total_size + 1;
    char* file_data = malloc(*result_size);
    if (!file_data) return -1;

    char* curr_ptr = file_data;

    for (size_t i = 0; i < (size_t)conf->numrows; i++) {
        memcpy(curr_ptr, conf->rows[i].chars, conf->rows[i].size);
        curr_ptr += conf->rows[i].size;
        *curr_ptr = '\n';
        curr_ptr++;
    }

    *curr_ptr = '\0';
    *result = file_data;

    return SUCCESS;
}

int editor_string_to_rows(struct EditorConfig* conf, char* buffer) {
    // reset all rows in conf
    conf_destroy_rows(conf);

    char* str = strdup(buffer);
    size_t index = 0, capacity = 10;
    conf->rows = calloc(sizeof(struct Row), capacity);

    char* token = strtok(str, "\n");

    while (token) {
        if (index >= capacity) {
            capacity *= 2;
            conf->rows = realloc(conf->rows, sizeof(struct Row) * capacity);
        }

        conf->rows[index].chars = strdup(token);
        conf->rows[index].idx = index;
        conf->rows[index].size = strlen(conf->rows[index].chars);
        conf->rows[index].render = NULL;
        conf->rows[index].rsize = 0;
        conf->rows[index].hl = NULL;
        conf->rows[index].hl_open_comment = false;

        conf->is_dirty = 1;
        editor_update_row(conf, &conf->rows[index]);

        token = strtok(NULL, "\n");
        index++;
    }

    if (index < capacity) {
        conf->rows = realloc(conf->rows, sizeof(struct Row) * index);
    }

    free(str);
    conf->numrows = index - 1;

    return SUCCESS;
}

int editor_row_append_string(struct EditorConfig* conf, struct Row* row,
                             char* s, size_t slen) {
    row->chars = realloc(row->chars, row->size + slen + 1);
    memcpy(&row->chars[row->size], s, slen);
    row->size += slen;
    row->chars[row->size] = '\0';
    editor_update_row(conf, row);
    conf->is_dirty = 1;

    return SUCCESS;
}

int editor_update_cx_rx(struct Row* row, int cx) {
    int rx = 0;

    // numline offset effect
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
    int numline_offset_size = editor_row_numline_calculate(row);
    int indent = row->indentation;  // modified indentation (if needed)

    bool in_string = false;

    // stack will work great to know if an indentation is essential

    Stack* s = malloc(sizeof(Stack));
    stack_create(s, NULL, free);

    for (size_t i = 0; i < (size_t)conf->cx - numline_offset_size; i++) {
        char c = row->chars[i];

        if (c == '"' && (i == 0 || row->chars[i - 1] != '\\')) {
            in_string = !in_string;
            continue;
        }
        if (!in_string) {
            char* data;
            if (c == '{') {
                data = strdup("{");
                stack_push(s, data);
            } else if (c == '}') {
                char* peaked = stack_peek(s);
                void* ptr;
                if (peaked && strcmp(peaked, "{") == 0) {
                    stack_pop(s, &ptr);
                    free(ptr);
                } else {
                    data = strdup("}");
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
        *data == '{' ? indent++ : indent--;
        curr = curr->next;
        free(data);
    }

    indent = indent < 0 ? 0 : indent;  // verify it's not less than 0

    free(s);

    int remainder_len = row->size - conf->cx + numline_offset_size;
    char* remainder = &row->chars[conf->cx - numline_offset_size];

    char* newline = malloc(remainder_len + indent + 1);
    if (!newline) return OUT_OF_MEMORY;

    memset(newline, '\t', indent);
    memcpy(&newline[indent], remainder, remainder_len);
    newline[remainder_len + indent] = '\0';

    *data = newline;
    *len = remainder_len + indent;

    return SUCCESS;
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
