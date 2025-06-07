#ifndef OPERATIONS_H
#define OPERATIONS_H

#include <stdbool.h>
#include <stddef.h>

struct Config;
enum EditorHighlight;

struct e_row {
    char* chars;
    char* render;
    unsigned char* hl;  // stands for highlighting

    int idx;
    size_t size;
    size_t rsize;
    bool hl_open_comment;
};

int editor_free_row(struct e_row* row);
int editor_insert_row_char(struct Config* conf, struct e_row* row, int at,
                           int c);
int editor_delete_row_char(struct Config* conf, struct e_row* row, int at);
int editor_insert_row(struct Config* conf, int at, const char* content,
                      size_t content_size);

int editor_update_row(struct Config* conf, struct e_row* row);
int editor_delete_row(struct Config* conf, int at);

int editor_insert_char(struct Config* conf, int c);
int editor_delete_char(struct Config* conf);

int editor_rows_to_string(struct Config* conf, char** result,
                          size_t* result_size);
int editor_row_append_string(struct Config* conf, struct e_row* row, char* s,
                             size_t slen);

int editor_update_cx_rx(struct e_row* row, int cx);
int editor_update_rx_cx(struct e_row* row, int rx);

int editor_row_numline_calculate(struct e_row* row);

#endif