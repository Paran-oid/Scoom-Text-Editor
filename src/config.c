#include "config.h"

#include <stdlib.h>
#include <string.h>

#include "dlist.h"
#include "file.h"
#include "rows.h"
#include "terminal.h"

static int app_free(void* el) {
    free(el);
    return 0;
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

    conf->history = malloc(sizeof(struct DList));
    dlist_create(conf->history, sizeof(struct State), app_free, app_cmp);

    if (term_get_window_size(conf, &conf->screen_rows, &conf->screen_cols) !=
        0) {
        return -1;
    }

    conf->screen_rows -= 2;

    return 0;
}

int config_destroy(struct Config* conf) {
    if (conf->filename) free(conf->filename);

    for (size_t i = 0; i < (size_t)conf->numrows; i++) {
        free(conf->rows[i].chars);
        free(conf->rows[i].render);
    }

    free(conf->history);
    free(conf->rows);
    free(conf);

    return 0;
}