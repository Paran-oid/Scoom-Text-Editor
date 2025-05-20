#ifndef INOUT_H
#define INOUT_H

#define CTRL_KEY(c) ((c) & 0x1f)

struct editorConfig;
struct abuf;
typedef unsigned long size_t;

int editor_create(struct editorConfig *conf);

#define ABUF_INIT \
    { NULL, 0 }
int ab_append(struct abuf *ab, const char *s, size_t len);
int ab_free(struct abuf *ab);

int editor_refresh_screen(struct editorConfig *conf);
int editor_draw_rows(struct editorConfig *conf, struct abuf *ab);

char editor_read_key(void);
int editor_process_key_press(void);

#endif