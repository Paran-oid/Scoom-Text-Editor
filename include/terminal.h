#ifndef TERMINAL_H
#define TERMINAL_H

struct EditorConfig;

#define SCROLL_DISABLE 0
#define MOUSE_REPORTING 0

__attribute__((unused)) static struct EditorConfig* g_conf;

void term_create(struct EditorConfig* conf);
void term_exit(int sig);

int term_get_window_size(struct EditorConfig* conf, int* rows, int* cols);
int term_get_cursor_position(int* rows, int* cols);

#endif