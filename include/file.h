#ifndef FILE_H
#define FILE_H

struct Config;
typedef unsigned long size_t;

void editor_open(struct Config* conf, const char* path);
void editor_append_row(struct Config* conf, const char* content,
                       size_t content_size);

#endif