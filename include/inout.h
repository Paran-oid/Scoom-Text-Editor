#ifndef INOUT_H
#define INOUT_H

#define CTRL_KEY(c) ((c) & 0x1f)

struct Config;
struct abuf;
enum EditorKey;

typedef unsigned long size_t;

#define ABUF_INIT \
    { NULL, 0 }

int ab_append(struct abuf *ab, const char *s, size_t len);
int ab_free(struct abuf *ab);

char *editor_prompt(struct Config *conf, const char *prompt,
                    void (*callback)(struct Config *, char *, int));
int editor_set_status_message(struct Config *conf, const char *message, ...);
int editor_refresh_screen(struct Config *conf);

int editor_draw_messagebar(struct Config *conf, struct abuf *ab);
int editor_draw_statusbar(struct Config *conf, struct abuf *ab);
int editor_draw_rows(struct Config *conf, struct abuf *ab);

int editor_insert_newline(struct Config *conf);
int editor_scroll(struct Config *conf);
int editor_cursor_shift(struct Config *conf, enum EditorKey key);
int editor_cursor_move(struct Config *conf, int key);

int editor_read_key(void);
int editor_process_key_press(struct Config *conf);

#endif