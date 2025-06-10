#include "config.h"

#include <stdlib.h>

#include "dlist.h"
#include "rows.h"
#include "terminal.h"

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