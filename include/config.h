#ifndef CONFIG_H
#define CONFIG_H

#include <stack.h>
#include <stdbool.h>
#include <termios.h>
#include <time.h>

#define SCOOM_VERSION 0.01
#define TAB_SIZE 4
#define QUIT_TIMES 3
#define IDENT_SIZE 4

struct State;
struct DList;

struct Config {
    // termios related
    struct termios termios;

    // status bar
    char sbuf[80];  // status buffer
    time_t sbuf_time;

    // cursor related
    int cx;
    int cy;
    int rx;

    int screen_rows;
    int screen_cols;

    // file handling
    char* filename;
    struct Row* rows;
    struct EditorSyntax* syntax;
    int numrows;
    int rowoff, coloff;
    unsigned int dirty : 1;

    // saves state of application for undo and redos and so forth
    Stack* stack_undo;
    Stack* stack_redo;
};

int config_create(struct Config* conf);
int conf_to_state_update(struct Config* conf, struct State* state);
int config_destroy(struct Config* conf);

#endif