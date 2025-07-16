#ifndef INOUT_H
#define INOUT_H

#include <stdint.h>
struct EditorConfig;
struct ABuf;
enum EditorKey;

uint8_t editor_refresh_screen(struct EditorConfig *conf);
uint8_t editor_scroll(struct EditorConfig *conf);

uint8_t editor_draw_messagebar(struct EditorConfig *conf, struct ABuf *ab);
uint8_t editor_draw_statusbar(struct EditorConfig *conf, struct ABuf *ab);
uint8_t editor_draw_rows(struct EditorConfig *conf, struct ABuf *ab);

#endif