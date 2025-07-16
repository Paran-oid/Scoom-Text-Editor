#ifndef INPUT_H
#define INPUT_H

#include <stdint.h>
struct EditorConfig;
struct Row;

enum EditorKey {
    BACKSPACE = 127,

    ARROW_UP = 1000,
    ARROW_DOWN,
    ARROW_RIGHT,
    ARROW_LEFT,

    SHIFT_ARROW_UP,
    SHIFT_ARROW_DOWN,
    SHIFT_ARROW_RIGHT,
    SHIFT_ARROW_LEFT,

    CTRL_ARROW_UP,
    CTRL_ARROW_DOWN,
    CTRL_ARROW_RIGHT,
    CTRL_ARROW_LEFT,

    PAGE_UP,
    PAGE_DOWN,

    HOME_KEY,
    DEL_KEY,
    END_KEY,

    F1,
    F2,
    F3,
    F4,
    F5,
    F6,
    F7,
    F8,
    F9,
    F10,
    F11,
    F12,

};

uint8_t editor_set_status_message(struct EditorConfig *conf,
                                  const char *message, ...);
char *editor_prompt(struct EditorConfig *conf, const char *prompt,
                    void (*callback)(struct EditorConfig *, char *, int32_t));

uint8_t editor_cursor_ctrl(struct EditorConfig *conf, enum EditorKey key);
uint8_t editor_cursor_move(struct EditorConfig *conf, enum EditorKey key);
uint8_t editor_shift_select(struct EditorConfig *conf, enum EditorKey key);

uint8_t editor_read_key(struct EditorConfig *conf);
uint8_t editor_process_key_press(struct EditorConfig *conf);

uint8_t editor_insert_newline(struct EditorConfig *conf);

#endif