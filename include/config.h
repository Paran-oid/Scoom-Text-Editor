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

struct EditorConfig {
    // termios related
    struct termios orig_termios;

    // status bar
    char status_msg[80];  // status buffer
    time_t sbuf_time;

    // cursor related
    int cx, cy;  // cursor position in characters
    int rx;      // rendered x position, accounting for tabs

    int screen_rows;
    int screen_cols;

    // file handling
    char* filename;
    struct Row* rows;  // array of rows
    struct EditorSyntax* syntax;
    int numrows;
    int rowoff, coloff;
    unsigned int is_dirty : 1;

    // saves snapshot of application for undo and redos and so forth
    Stack* stack_undo;
    Stack* stack_redo;
    time_t last_time_modified;  // last time modified
};

enum EditorStatus {

    // comments generated with AI for clarity

    // General statuses
    SUCCESS = 0,
    ERROR,
    INVALID_ARG,
    OUT_OF_MEMORY,

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
    EMPTY_BUFFER,
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