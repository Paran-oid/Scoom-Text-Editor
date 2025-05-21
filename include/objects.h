#ifndef OBJECTS_H
#define OBJECTS_H

#include <stddef.h>
#include <termios.h>

#define SCOOM_VERSION 0.01

struct editorConfig {
    struct termios termios;

    // cursor positions;
    int cx;
    int cy;

    int screen_rows;
    int screen_cols;
};

struct abuf {
    char *buf;
    size_t len;
};

enum EditorKey { ARROW_UP = 1000, ARROW_DOWN, ARROW_RIGHT, ARROW_LEFT };

#endif