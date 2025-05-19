#include "inout.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "objects.h"
#include "terminal.h"

int editor_create(struct editorConfig *conf) {
    if (term_get_window_size(conf) != 0) {
        // C: cursor forward(continue)
        // B: cursor down(bottom)
        if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return -1;
        editor_read_key();
        return -1;
    }
    return 0;
}

// OUTPUT

int editor_refresh_screen(struct editorConfig *conf) {
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);

    editor_draw_rows(conf);
    write(STDOUT_FILENO, "\x1b[H", 3);

    return 0;
}

int editor_draw_rows(struct editorConfig *conf) {
    for (size_t i = 0; i < conf->screen_rows; i++) {
        write(STDOUT_FILENO, "~\r\n", 3);
    }

    return 0;
}

// INPUT

char editor_read_key(void) {
    char c;
    int nread;
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if (nread == -1 && errno != EAGAIN) die("read");
    }
    return c;
}
int editor_process_key_press(void) {
    char c = editor_read_key();
    switch (c) {
        case CTRL_KEY('q'):
            write(STDOUT_FILENO, "\x1b[2J", 4);
            write(STDOUT_FILENO, "\x1b[H", 3);
            exit(1);
            break;
        default:
            printf("%c\r\n", c);
            break;
    }

    return 0;
}