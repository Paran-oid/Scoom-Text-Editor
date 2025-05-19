#ifndef INOUT_H
#define INOUT_H

#define CTRL_KEY(c) ((c) & 0x1f)

struct editorConfig;

int editor_create(struct editorConfig *conf);

int editor_refresh_screen(struct editorConfig *conf);
int editor_draw_rows(struct editorConfig *conf);

char editor_read_key(void);
int editor_process_key_press(void);

#endif