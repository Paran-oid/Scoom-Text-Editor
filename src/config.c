#include "config.h"

#include <stdlib.h>
#include <string.h>

#include "core.h"
#include "file.h"
#include "rows.h"
#include "terminal.h"

static void app_destroy(void* el) {
    if (snapshot_destroy((struct Snapshot*)el) != SUCCESS) {
        die("couldn't destroy snapshot");
    }
    free(el);
}

static int app_cmp(const void* str1, const void* str2) {
    return strcmp((const char*)str1, (const char*)str2);
}

int conf_create(struct EditorConfig* conf) {
    conf->filepath = NULL;

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
    conf->is_dirty = 0;
    conf->syntax = NULL;
    conf->resize_needed = 0;

    conf->stack_undo = malloc(sizeof(Stack));
    conf->stack_redo = malloc(sizeof(Stack));

    if (!conf->stack_undo || !conf->stack_redo) return OUT_OF_MEMORY;

    conf->last_time_modified = time(NULL);

    stack_create(conf->stack_undo, app_cmp, app_destroy);
    stack_create(conf->stack_redo, app_cmp, app_destroy);

    if (term_get_window_size(conf, &conf->screen_rows, &conf->screen_cols) !=
        0) {
        return -1;
    }

    conf->sel.start_row = -1;
    conf->sel.start_col = -1;
    conf->sel.end_row = -1;
    conf->sel.end_col = -1;
    conf->screen_rows -= 2;

    return SUCCESS;
}

int conf_to_snapshot_update(struct EditorConfig* conf,
                            struct Snapshot* snapshot) {
    enum EditorStatus retval;

    conf->cx = snapshot->cx;
    conf->cy = snapshot->cy;

    if ((retval = editor_string_to_rows(conf, snapshot->text)) != SUCCESS) {
        return retval;
    }

    conf->numrows = snapshot->numrows;

    // after consuming it just delete it to save some memory
    snapshot_destroy(snapshot);

    return SUCCESS;
}

int conf_destroy_rows(struct EditorConfig* conf) {
    for (size_t i = 0; i < (size_t)conf->numrows; i++) {
        free(conf->rows[i].chars);
        free(conf->rows[i].render);
        free(conf->rows[i].hl);
    }
    free(conf->rows);

    conf->rows = NULL;

    return SUCCESS;
}

int conf_destroy(struct EditorConfig* conf) {
    if (conf->filepath) free(conf->filepath);

    conf_destroy_rows(conf);

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
    conf->is_dirty = 0;

    conf->status_msg[0] = '\0';
    conf->sbuf_time = 0;
    conf->last_time_modified = 0;

    conf->sel.active = 0;
    conf->sel.start_row = -1;
    conf->sel.start_col = -1;
    conf->sel.end_row = -1;
    conf->sel.end_col = -1;

    memset(&conf->orig_termios, 0, sizeof(conf->orig_termios));

    return SUCCESS;
}