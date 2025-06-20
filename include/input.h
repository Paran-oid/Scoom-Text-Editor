#ifndef INPUT_H
#define INPUT_H

struct EditorConfig;
struct Row;

enum EditorKey {
    BACKSPACE = 127,

    ARROW_UP = 1000,
    ARROW_DOWN,
    ARROW_RIGHT,
    ARROW_LEFT,

    CTRL_ARROW_UP,
    CTRL_ARROW_DOWN,
    CTRL_ARROW_RIGHT,
    CTRL_ARROW_LEFT,

    PAGE_UP,
    PAGE_DOWN,

    HOME_KEY,
    DEL_KEY,
    END_KEY
};

int editor_set_status_message(struct EditorConfig *conf, const char *message,
                              ...);
char *editor_prompt(struct EditorConfig *conf, const char *prompt,
                    void (*callback)(struct EditorConfig *, char *, int));

int editor_cursor_cursor(struct EditorConfig *conf, enum EditorKey key);
int editor_cursor_move(struct EditorConfig *conf, int key);

int editor_read_key(void);
int editor_process_key_press(struct EditorConfig *conf);

int editor_insert_newline(struct EditorConfig *conf);

#endif