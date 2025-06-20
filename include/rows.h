#ifndef OPERATIONS_H
#define OPERATIONS_H

#include <stdbool.h>
#include <stddef.h>

struct EditorConfig;
enum EditorHighlight;

struct Row {
    char* chars;
    char* render;
    unsigned char* hl;  // stands for highlighting

    int idx;
    size_t indentation;
    size_t size;
    size_t rsize;
    bool hl_open_comment;
};

int editor_free_row(struct Row* row);
int editor_insert_row_char(struct EditorConfig* conf, struct Row* row, int at,
                           int c);
int editor_delete_row_char(struct EditorConfig* conf, struct Row* row, int at);
int editor_insert_row(struct EditorConfig* conf, int at, const char* content,
                      size_t content_size);


int editor_modify_row(struct EditorConfig* conf, struct Row* row);
int editor_update_row(struct EditorConfig* conf, struct Row* row);
int editor_delete_row(struct EditorConfig* conf, int at);

int editor_insert_char(struct EditorConfig* conf, int c);
int editor_delete_char(struct EditorConfig* conf);

int editor_rows_to_string(struct EditorConfig* conf, char** result,
                          size_t* result_size);

int editor_string_to_rows(struct EditorConfig* conf, char* buffer);

int editor_row_append_string(struct EditorConfig* conf, struct Row* row,
                             char* s, size_t slen);

/*
 * return newline formed in newline paramter
 * set it's length in len parameter
 */
int editor_row_indent(struct EditorConfig* conf, struct Row* row,
                      char** newline, size_t* len);

int editor_update_cx_rx(struct Row* row, int cx);
int editor_update_rx_cx(struct Row* row, int rx);

int editor_row_numline_calculate(struct Row* row);

#endif