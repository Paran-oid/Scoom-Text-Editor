#ifndef INOUT_H
#define INOUT_H

struct Config;
struct ABuf;
enum EditorKey;

typedef unsigned long size_t;

#define ABUF_INIT \
    { NULL, 0 }

int editor_refresh_screen(struct Config *conf);
int editor_scroll(struct Config *conf);

int editor_draw_messagebar(struct Config *conf, struct ABuf *ab);
int editor_draw_statusbar(struct Config *conf, struct ABuf *ab);
int editor_draw_rows(struct Config *conf, struct ABuf *ab);

#endif