#ifndef OBJECTS_H
#define OBJECTS_H

#include <stddef.h>
#include <termios.h>

struct editorConfig {
    struct termios termios;

    int screen_rows;
    int screen_cols;
};

struct abuf {
    char *buf;
    size_t len;
};
#endif