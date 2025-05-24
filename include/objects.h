#ifndef OBJECTS_H
#define OBJECTS_H

#include <stddef.h>
#include <termios.h>

#define SCOOM_VERSION 0.01
#define TAB_SIZE 4

typedef long int time_t;

// appending buffer
struct abuf {
    char* buf;
    int len;
};

// editor row
struct e_row {
    char* chars;
    size_t size;

    char* render;
    size_t rsize;
};

struct Config {
    struct termios termios;
    char* filename;

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

    // for text management
    struct e_row* rows;
    int numrows;
    int rowoff, coloff;
};

enum EditorKey {
    ARROW_UP = 1000,
    ARROW_DOWN,
    ARROW_RIGHT,
    ARROW_LEFT,

    PAGE_UP,
    PAGE_DOWN,

    HOME_KEY,
    DEL_KEY,
    END_KEY
};

#endif