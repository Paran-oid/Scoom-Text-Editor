#ifndef INOUT_H
#define INOUT_H

struct EditorConfig;
struct ABuf;
enum EditorKey;

typedef unsigned long size_t;

int editor_refresh_screen(struct EditorConfig *conf);
int editor_scroll(struct EditorConfig *conf);

int editor_draw_messagebar(struct EditorConfig *conf, struct ABuf *ab);
int editor_draw_statusbar(struct EditorConfig *conf, struct ABuf *ab);
int editor_draw_rows(struct EditorConfig *conf, struct ABuf *ab);

#endif