#ifndef FILE_H
#define FILE_H

#define DEBUG

#include <stddef.h>

struct Config;

enum StateType { STATE_DELETE, STATE_INSERT };

struct State {
    char* text;
    enum StateType type;

    int cx;
    int cy;
};

int state_create(struct State* state, int cx, int cy, enum StateType type,
                 const char* content, size_t size);
int state_destroy(struct State* state);

int editor_open(struct Config* conf, const char* path);
int editor_run(struct Config* conf);
int editor_destroy(struct Config* conf);
int editor_save(struct Config* conf);

int editor_undo(struct Config* conf);
int editor_redo(struct Config* conf);

int editor_copy(struct Config* conf);
int editor_paste(struct Config* conf);
int editor_cut(struct Config* conf);

int editor_find(struct Config* conf);

#endif