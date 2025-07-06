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

struct Snapshot;
struct DList;

struct EditorCursorSelect {
    int active;
    int start_row;
    int start_col;
    int end_row;
    int end_col;
};

struct EditorConfig {
    // Pointers (8 bytes each on 64-bit)
    char* filepath;
    struct Row* rows;
    struct EditorSyntax* syntax;
    Stack* stack_undo;
    Stack* stack_redo;

    // time_t (usually 8 bytes)
    time_t sbuf_time;
    time_t last_time_modified;

    // Integers (4 bytes each)
    int cx, cy;  // cursor
    int rx;      // rendered x (for tabs)
    int screen_rows, screen_cols;
    int numrows;
    int rowoff, coloff;

    // bitfields packed into one 4-byte int
    unsigned int is_dirty : 1;
    unsigned int resize_needed : 1;

    // char array (80 bytes)
    char status_msg[80];

    // Terminal config (termios is usually ~60 bytes, well-aligned)
    struct termios orig_termios;
    struct EditorCursorSelect sel;
};

enum EditorStatus {

    // comments generated with AI for clarity

    // General statuses
    SUCCESS = 0,
    ERROR,
    NULL_PARAMETER,
    INVALID_ARG,
    OUT_OF_MEMORY,
    CURSOR_PRESS,
    INTERRUPT_ENCOUNTERED = 666,

    // File-related errors
    FILE_NOT_FOUND,
    FILE_OPEN_FAILED,
    FILE_WRITE_FAILED,
    FILE_READ_FAILED,
    FILE_TOO_LARGE,
    FILE_PERMISSION_DENIED,
    FILE_ENCODING_UNSUPPORTED,

    // Editor-specific errors
    SYNTAX_ERROR,
    CURSOR_OUT_OF_BOUNDS,
    CURSOR_RELEASE,
    EMPTY_BUFFER,
    EMPTY_COPY_BUFFER,
    UNSUPPORTED_OPERATION,

    // Stack or undo/redo issues
    UNDO_STACK_EMPTY,
    REDO_STACK_EMPTY,
    SNAPSHOT_FAILED,

    // Search-related
    SEARCH_NOT_FOUND,
    SEARCH_INVALID_REGEX,

    // Configuration/initialization
    CONFIG_LOAD_FAILED,
    CONFIG_SAVE_FAILED,

    // Exit codes
    EXIT_CODE = 100  // used just to leave the program's loop
};

int conf_create(struct EditorConfig* conf);
int conf_to_snapshot_update(struct EditorConfig* conf,
                            struct Snapshot* snapshot);
int conf_destroy_rows(struct EditorConfig* conf);
int conf_destroy(struct EditorConfig* conf);

#endif