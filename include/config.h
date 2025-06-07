#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>
#include <termios.h>
#include <time.h>

#define SCOOM_VERSION 0.01
#define TAB_SIZE 4
#define QUIT_TIMES 3
#define IDENT_SIZE 4

struct Config {
    // termios related
    struct termios termios;

    // status bar
    char sbuf[80];  // status buffer
    time_t sbuf_time;

    // cursor positions non rendered
    int cx;
    int cy;

    // cursor positions rendered
    int rx;

    int screen_rows;
    int screen_cols;

    // file handling
    char* filename;
    struct e_row* rows;
    struct EditorSyntax* syntax;
    int numrows;
    int rowoff, coloff;
    unsigned int dirty : 1;
};

int config_create(struct Config* conf);
int config_destroy(struct Config* conf);

#endif