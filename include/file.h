#ifndef FILE_H
#define FILE_H

#include "objects.h"
struct Config;
typedef unsigned long size_t;

int editor_open(struct Config* conf, const char* path);
int editor_append_row(struct Config* conf, const char* content,
                      size_t content_size);
int editor_update_cx_rx(struct e_row* row, int cx);
int editor_update_row(struct e_row* row);

#endif