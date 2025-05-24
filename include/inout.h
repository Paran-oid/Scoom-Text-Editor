#ifndef INOUT_H
#define INOUT_H

#define CTRL_KEY(c) ((c) & 0x1f)

struct Config;
struct abuf;
typedef unsigned long size_t;

int editor_create(struct Config *conf);

#define ABUF_INIT \
    { NULL, 0 }
int ab_append(struct abuf *ab, const char *s, size_t len);
int ab_free(struct abuf *ab);

int editor_refresh_screen(struct Config *conf);
int editor_draw_statusbar(struct Config *conf, struct abuf *ab);
int editor_draw_rows(struct Config *conf, struct abuf *ab);

int editor_scroll(struct Config *conf);
int editor_cursor_move(struct Config *conf, int key);

int editor_read_key(void);
int editor_process_key_press(struct Config *conf);

#endif