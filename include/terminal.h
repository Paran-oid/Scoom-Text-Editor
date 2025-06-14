#ifndef TERMINAL_H
#define TERMINAL_H

struct EditorConfig;

__attribute__((unused)) static void* temp;

void term_create(struct EditorConfig* conf);
void term_exit(void);

int term_get_window_size(struct EditorConfig* conf, int* rows, int* cols);
int term_get_cursor_position(int* rows, int* cols);

#endif