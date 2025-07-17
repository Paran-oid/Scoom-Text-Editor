#ifndef OPERATIONS_H
#define OPERATIONS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct EditorConfig;
enum EditorHighlight;
enum EditorKey;

struct Row {
    char* chars;
    char* render;
    unsigned char* hl;  // stands for highlighting

    int32_t indentation;
    int32_t size;
    int32_t rsize;

    int32_t idx;
    int8_t hl_open_comment;
};

int8_t editor_free_row(struct Row* row);
int8_t editor_insert_row_char(struct EditorConfig* conf, struct Row* row,
                              int32_t at, int32_t c);
int8_t editor_delete_row_char(struct EditorConfig* conf, struct Row* row,
                              int32_t at);
int8_t editor_insert_row(struct EditorConfig* conf, int32_t at,
                         const char* content, int32_t content_size);

int8_t editor_update_row(struct EditorConfig* conf, struct Row* row);
int8_t editor_delete_row(struct EditorConfig* conf, int32_t at);

int8_t editor_insert_char(struct EditorConfig* conf, int32_t c);
int8_t editor_delete_char(struct EditorConfig* conf);

int8_t editor_rows_to_string(struct EditorConfig* conf, char** result,
                             int32_t* result_size);

int8_t editor_string_to_rows(struct EditorConfig* conf, char* buffer);

int8_t editor_row_append_string(struct EditorConfig* conf, struct Row* row,
                                char* s, int32_t slen);

/*
 * return newline formed in newline paramter
 * set it's length in len parameter
 */
int8_t editor_row_indent(struct EditorConfig* conf, struct Row* row,
                         char** newline, int32_t* len);

int32_t editor_update_cx_rx(struct Row* row, int32_t cx);
int32_t editor_update_rx_cx(struct Row* row, int32_t rx);

int32_t editor_row_numline_calculate(const struct Row* row);

#endif