#include "config.h"

#include <stdlib.h>
#include <string.h>

#include "file.h"
#include "rows.h"
#include "terminal.h"

static void app_destroy(void* el) {
    struct Snapshot* s = (struct Snapshot*)el;
    free(s->text);
    free(s);
}

static int app_cmp(const void* str1, const void* str2) {
    return strcmp((const char*)str1, (const char*)str2);
}

int config_create(struct EditorConfig* conf) {
    conf->filename = NULL;

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

    conf->stack_undo = malloc(sizeof(Stack));
    conf->stack_redo = malloc(sizeof(Stack));
    conf->last_time_modified = time(NULL);

    stack_create(conf->stack_undo, app_cmp, app_destroy);
    stack_create(conf->stack_redo, app_cmp, app_destroy);

    if (term_get_window_size(conf, &conf->screen_rows, &conf->screen_cols) !=
        0) {
        return -1;
    }

    conf->screen_rows -= 2;

    return EXIT_SUCCESS;
}

int conf_to_snapshot_update(struct EditorConfig* conf,
                            struct Snapshot* snapshot) {
    if (snapshot->cy == conf->numrows) return EXIT_FAILURE;

    conf->cx = snapshot->cx;
    conf->cy = snapshot->cy;

    // TODO
    editor_string_to_rows(conf, snapshot->text, conf->rows, conf->numrows);

    snapshot_destroy(snapshot);

    return EXIT_SUCCESS;
}

int config_destroy(struct EditorConfig* conf) {
    if (conf->filename) free(conf->filename);

    for (size_t i = 0; i < (size_t)conf->numrows; i++) {
        free(conf->rows[i].chars);
        free(conf->rows[i].render);
    }

    stack_destroy(conf->stack_redo);
    stack_destroy(conf->stack_undo);

    free(conf->rows);
    free(conf);

    return EXIT_SUCCESS;
}