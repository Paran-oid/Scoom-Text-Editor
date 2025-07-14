#ifndef CONFIG_H
#define CONFIG_H

#include <stack.h>
#include <stdbool.h>
#include <termios.h>
#include <time.h>

#define SCOOM_VERSION 0.76
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
    unsigned int program_state : 1;  // 1-started or 0-finished

    // char array (80 bytes)
    char status_msg[80];

    // Terminal config (termios is usually ~60 bytes, well-aligned)
    struct termios orig_termios;
    struct EditorCursorSelect sel;
};

enum EditorCursorAnchor {
    CURSOR_ANCHOR_BEFORE,
    CURSOR_ANCHOR_AFTER,
    CURSOR_ANCHOR_AT,
    CURSOR_ANCHOR_INVALID
};

int conf_create(struct EditorConfig* conf);
int conf_destroy(struct EditorConfig* conf);

int conf_select_update(struct EditorConfig* conf, int start_row, int end_row,
                       int start_col, int end_col);
int conf_to_snapshot_update(struct EditorConfig* conf,
                            struct Snapshot* snapshot);
int conf_destroy_rows(struct EditorConfig* conf);

enum EditorCursorAnchor conf_check_cursor_anchor(struct EditorConfig* conf,
                                                 int anchor_row,
                                                 int anchor_col);
#endif