#include "config.h"

#include <stdlib.h>
#include <string.h>

#include "core.h"
#include "file.h"
#include "rows.h"
#include "terminal.h"

struct EditorConfig* g_conf = NULL;

static void app_destroy(void* el) {
    if (snapshot_destroy((struct Snapshot*)el) != 0)
        die("snapshot destroy failed");
    free(el);
}

static int32_t app_cmp(const void* str1, const void* str2) {
    return strcmp((const char*)str1, (const char*)str2);
}

uint8_t conf_create(struct EditorConfig* conf) {
    conf->filepath = NULL;
    // this is true only when user inputs something
    conf->flags.program_state = 0;

    // status message section
    conf->status_msg[0] = '\0';
    conf->sbuf_time = 0;

    // cursor section
    conf->cx = 2;
    conf->cy = 0;
    conf->rx = 0;

    conf->numrows = 0;
    conf->rows = NULL;
    conf->rowoff = 0;
    conf->coloff = 0;
    conf->flags.is_dirty = 0;
    conf->syntax = NULL;
    conf->flags.resize_needed = 0;

    conf->stack_undo = malloc(sizeof(Stack));
    conf->stack_redo = malloc(sizeof(Stack));

    if (!conf->stack_undo || !conf->stack_redo)
        die("malloc for stack_undo or stack_redo failed");

    conf->last_time_modified = time(NULL);
    if (conf->last_time_modified == -1) die("time funciton init failed");

    stack_create(conf->stack_undo, app_cmp, app_destroy);
    stack_create(conf->stack_redo, app_cmp, app_destroy);

    // even if it returned overflowed values it will execute die exit function
    if (term_get_window_size(conf, &conf->screen_rows, &conf->screen_cols) != 0)
        die("operation of retrieving window size failed");

    conf->sel.start_row = -1;
    conf->sel.start_col = -1;
    conf->sel.end_row = -1;
    conf->sel.end_col = -1;

    // inorder to have message bar and status bar we need to decrement by 2
    conf->screen_rows -= 2;

    return EXIT_SUCCESS;
}

uint8_t conf_select_update(struct EditorConfig* conf, int32_t start_row,
                           int32_t end_row, int32_t start_col,
                           int32_t end_col) {
    if (!conf) die("empty conf passed");

    conf->sel.start_col = start_col;
    conf->sel.start_row = start_row;
    conf->sel.end_col = end_col;
    conf->sel.end_row = end_row;

    return EXIT_SUCCESS;
}

uint8_t conf_to_snapshot_update(struct EditorConfig* conf,
                                struct Snapshot* snapshot) {
    conf->cx = snapshot->cx;
    conf->cy = snapshot->cy;

    if (editor_string_to_rows(conf, snapshot->text) != EXIT_SUCCESS)
        die("converting string to rows failed");

    conf->numrows = snapshot->numrows;

    if (snapshot_destroy(snapshot) != EXIT_SUCCESS)
        die("snapshot destroy failed");

    return EXIT_SUCCESS;
}

uint8_t conf_destroy_rows(struct EditorConfig* conf) {
    for (size_t i = 0; i < (size_t)conf->numrows; i++) {
        free(conf->rows[i].chars);
        free(conf->rows[i].render);
        free(conf->rows[i].hl);
    }

    free(conf->rows);
    conf->rows = NULL;

    return EXIT_SUCCESS;
}

uint8_t conf_destroy(struct EditorConfig* conf) {
    if (conf->filepath) free(conf->filepath);
    conf->flags.program_state = 0;

    if (conf_destroy_rows(conf) != EXIT_SUCCESS) die("destroy rows failed");

    stack_destroy(conf->stack_redo);
    stack_destroy(conf->stack_undo);

    // Reset all fields to safe values
    conf->cx = 0;
    conf->cy = 0;
    conf->rx = 0;

    conf->screen_rows = 0;
    conf->screen_cols = 0;

    conf->rows = NULL;
    conf->syntax = NULL;
    conf->numrows = 0;

    conf->rowoff = 0;
    conf->coloff = 0;
    conf->flags.is_dirty = 0;

    conf->status_msg[0] = '\0';
    conf->sbuf_time = 0;
    conf->last_time_modified = 0;

    conf->sel.active = 0;
    conf->sel.start_row = -1;
    conf->sel.start_col = -1;
    conf->sel.end_row = -1;
    conf->sel.end_col = -1;

    free(conf->stack_redo);
    free(conf->stack_undo);

    return EXIT_SUCCESS;
}

enum EditorCursorAnchor conf_check_cursor_anchor(struct EditorConfig* conf,
                                                 int32_t anchor_row,
                                                 int32_t anchor_col) {
    if (conf->cy < anchor_row) {
        return CURSOR_ANCHOR_BEFORE;
    } else if (conf->cy == anchor_row && conf->rx < anchor_col) {
        return CURSOR_ANCHOR_BEFORE;
    } else if (conf->cy == anchor_row && conf->rx == anchor_col) {
        return CURSOR_ANCHOR_AT;
    } else {
        return CURSOR_ANCHOR_AFTER;
    }
    return CURSOR_ANCHOR_INVALID;
}