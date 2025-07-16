#ifndef CONFIG_H
#define CONFIG_H

#include <stack.h>
#include <stdbool.h>
#include <stdint.h>
#include <termios.h>
#include <time.h>

#define DEBUG_MODE 1

#define SCOOM_VERSION 0.89
#define TAB_SIZE 4
#define QUIT_TIMES 3
#define IDENT_SIZE 4

#define EXIT_LOOP_CODE 0xA1
#define INTERRUPT_ENCOUNTERED 0xA2

struct Snapshot;
struct DList;

struct EditorCursorSelect {
    uint8_t active : 1;
    int32_t start_row;
    int32_t start_col;
    int32_t end_row;
    int32_t end_col;
};

struct EditorConfigFlags {
    uint8_t is_dirty : 1;
    uint8_t resize_needed : 1;
    uint8_t program_state : 1;  // 1-started or 0-finished
};

struct EditorConfig {
    char* filepath;
    struct Row* rows;
    struct EditorSyntax* syntax;
    Stack* stack_undo;
    Stack* stack_redo;

    time_t sbuf_time;
    time_t last_time_modified;

    uint32_t cx, cy;
    uint32_t rx;
    uint32_t screen_rows, screen_cols;
    uint32_t rowoff, coloff;
    size_t numrows;

    char status_msg[80];

    struct EditorConfigFlags flags;
    struct EditorCursorSelect sel;
};

enum EditorCursorAnchor {
    CURSOR_ANCHOR_BEFORE,
    CURSOR_ANCHOR_AFTER,
    CURSOR_ANCHOR_AT,
    CURSOR_ANCHOR_INVALID
};

extern struct EditorConfig* g_conf;

uint8_t conf_create(struct EditorConfig* conf);
uint8_t conf_destroy(struct EditorConfig* conf);

uint8_t conf_select_update(struct EditorConfig* conf, int32_t start_row,
                           int32_t end_row, int32_t start_col, int32_t end_col);
uint8_t conf_to_snapshot_update(struct EditorConfig* conf,
                                struct Snapshot* snapshot);
uint8_t conf_destroy_rows(struct EditorConfig* conf);

enum EditorCursorAnchor conf_check_cursor_anchor(struct EditorConfig* conf,
                                                 int32_t anchor_row,
                                                 int32_t anchor_col);
#endif