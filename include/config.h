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

int conf_create(struct EditorConfig* conf);
int conf_to_snapshot_update(struct EditorConfig* conf,
                            struct Snapshot* snapshot);
int conf_destroy_rows(struct EditorConfig* conf);
int conf_destroy(struct EditorConfig* conf);

#endif