#ifndef OBJECTS_H
#define OBJECTS_H

#include <stddef.h>
#include <termios.h>

struct editorConfig {
    struct termios termios;

    size_t screen_rows;
    size_t screen_cols;
};

#endif