#ifndef FILE_H
#define FILE_H

#include "objects.h"
struct Config;

int editor_open(struct Config* conf, const char* path);
int editor_save(struct Config* conf);

#endif