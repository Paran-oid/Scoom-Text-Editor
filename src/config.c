#include "config.h"

#include <stdlib.h>
#include <string.h>

#include "file.h"
#include "rows.h"
#include "terminal.h"

static void app_destroy(void* el) {
    struct State* s = (struct State*)el;
    free(s->text);
    free(s);
}

static int app_cmp(const void* str1, const void* str2) {
    return strcmp((const char*)str1, (const char*)str2);
}

int config_create(struct Config* conf) {
    conf->filename = NULL;

    // status message section
    conf->sbuf[0] = '\0';
    conf->sbuf_time = 0;

    // cursor section
    conf->cx = 2;
    conf->cy = 0;
    conf->rx = 0;

    conf->numrows = 0;
    conf->rows = NULL;
    conf->rowoff = 0;
    conf->coloff = 0;
    conf->dirty = 0;
    conf->syntax = NULL;

    conf->stack_undo = malloc(sizeof(Stack));
    conf->stack_redo = malloc(sizeof(Stack));

    stack_create(conf->stack_undo, app_cmp, app_destroy);
    stack_create(conf->stack_redo, app_cmp, app_destroy);

    if (term_get_window_size(conf, &conf->screen_rows, &conf->screen_cols) !=
        0) {
        return -1;
    }

    conf->screen_rows -= 2;

    return EXIT_SUCCESS;
}

int conf_to_state_update(struct Config* conf, struct State* state) {
    if (state->cy == conf->numrows) return EXIT_FAILURE;

    conf->cx = state->cx;
    conf->cy = state->cy;

    struct Row* row = &conf->rows[conf->cy];
    free(row->chars);

    row->chars = malloc(state->text_size + 1);
    memcpy(row->chars, state->text,
           state->text_size + 1);  // + 1 for null terminator
    row->size = state->text_size;

    editor_update_row(conf, row);

    return EXIT_SUCCESS;
}

int config_destroy(struct Config* conf) {
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