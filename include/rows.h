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

    size_t indentation;
    size_t size;
    size_t rsize;

    uint32_t idx;
    uint8_t hl_open_comment : 1;
};

uint8_t editor_free_row(struct Row* row);
uint8_t editor_insert_row_char(struct EditorConfig* conf, struct Row* row,
                               uint32_t at, enum EditorKey c);
uint8_t editor_delete_row_char(struct EditorConfig* conf, struct Row* row,
                               uint32_t at);
uint8_t editor_insert_row(struct EditorConfig* conf, uint32_t at,
                          const char* content, size_t content_size);

uint8_t editor_update_row(struct EditorConfig* conf, struct Row* row);
uint8_t editor_delete_row(struct EditorConfig* conf, uint32_t at);

uint8_t editor_insert_char(struct EditorConfig* conf, enum EditorKey c);
uint8_t editor_delete_char(struct EditorConfig* conf);

uint8_t editor_rows_to_string(struct EditorConfig* conf, char** result,
                              size_t* result_size);

uint8_t editor_string_to_rows(struct EditorConfig* conf, char* buffer);

uint8_t editor_row_append_string(struct EditorConfig* conf, struct Row* row,
                                 char* s, size_t slen);

/*
 * return newline formed in newline paramter
 * set it's length in len parameter
 */
uint8_t editor_row_indent(struct EditorConfig* conf, struct Row* row,
                          char** newline, size_t* len);

uint8_t editor_update_cx_rx(struct Row* row, uint32_t cx);
uint8_t editor_update_rx_cx(struct Row* row, uint32_t rx);

uint8_t editor_row_numline_calculate(const struct Row* row);

#endif