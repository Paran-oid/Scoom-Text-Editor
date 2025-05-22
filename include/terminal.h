#ifndef TERMINAL_H
#define TERMINAL_H

struct Config;

static void* temp;

void die(const char* s);

void term_create(struct Config* conf);
void term_exit(void);

int term_get_window_size(struct Config* conf, int* rows, int* cols);
int term_get_cursor_position(struct Config* conf, int* rows, int* cols);

#endif