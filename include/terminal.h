#ifndef TERMINAL_H
#define TERMINAL_H

#include <stdint.h>
struct EditorConfig;

#define SCROLL_DISABLE 1
#define MOUSE_REPORTING 0

extern struct termios orig_termios;

void term_create(void);
void term_exit(int32_t sig);

uint8_t term_get_window_size(struct EditorConfig* conf, uint32_t* rows,
                             uint32_t* cols);
uint8_t term_get_cursor_position(uint32_t* rows, uint32_t* cols);

#endif