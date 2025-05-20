#ifndef TERMINAL_H
#define TERMINAL_H

struct editorConfig;

static void* temp;

void die(const char* s);

void term_create(struct editorConfig* conf);
void term_exit(void);

int term_get_window_size(struct editorConfig* conf, int* rows, int* cols);
int term_get_cursor_position(struct editorConfig* conf, int* rows, int* cols);

#endif