#ifndef FILE_H
#define FILE_H

#include "objects.h"
struct Config;

int editor_open(struct Config* conf, const char* path);
int editor_save(struct Config* conf);

int editor_copy(struct Config* conf);
int editor_paste(struct Config* conf);
int editor_cut(struct Config* conf);

#endif