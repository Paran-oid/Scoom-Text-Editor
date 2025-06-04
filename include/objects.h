#ifndef OBJECTS_H
#define OBJECTS_H

#include <stdbool.h>
#include <stddef.h>
#include <termios.h>

#define SCOOM_VERSION 0.01
#define TAB_SIZE 4
#define QUIT_TIMES 3

typedef long int time_t;

// appending buffer
struct abuf {
    char* buf;
    int len;
};

// editor row
struct e_row {
    int idx;
    char* chars;
    size_t size;
    unsigned char* hl;  // stands for highlighting

    char* render;
    size_t rsize;
    bool hl_open_comment;
};

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

enum EditorKey {
    BACKSPACE = 127,

    ARROW_UP = 1000,
    ARROW_DOWN,
    ARROW_RIGHT,
    ARROW_LEFT,

    CTRL_ARROW_UP,
    CTRL_ARROW_DOWN,
    CTRL_ARROW_RIGHT,
    CTRL_ARROW_LEFT,

    PAGE_UP,
    PAGE_DOWN,

    HOME_KEY,
    DEL_KEY,
    END_KEY
};

enum EditorHighlight {
    HL_NORMAL = 0,
    HL_NUMBER,
    HL_MATCH,
    HL_STRING,
    HL_COMMENT,
    HL_MCOMMENT,
    HL_KEYWORD1,
    HL_KEYWORD2
};

struct EditorSyntax {
    char* filetype;
    char** filematch;
    char** keywords;
    int flags;
    char* singleline_comment_start;
    char* multiline_comment_start;
    char* multiline_comment_end;
};

#endif