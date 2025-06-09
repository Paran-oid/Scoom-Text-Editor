#ifndef FILE_H
#define FILE_H

#define DEBUG

struct Config;

struct State {};

int editor_open(struct Config* conf, const char* path);
int editor_run(struct Config* conf);
int editor_destroy(struct Config* conf);
int editor_save(struct Config* conf);

int editor_copy(struct Config* conf);
int editor_paste(struct Config* conf);
int editor_cut(struct Config* conf);

int editor_find(struct Config* conf);

#endif