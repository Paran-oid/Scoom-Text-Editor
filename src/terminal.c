#include "terminal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include "objects.h"

// usage of each macro can be understood in termios.h docs

// TCSAFLUSH: flushes before leaving the program

void die(const char* s) {
    perror(s);
    exit(1);
}

void term_create(struct editorConfig* conf) {
    if (tcgetattr(STDIN_FILENO, &conf->termios) == -1) {
        die("tcgetattr");
    }
    temp = malloc(sizeof(struct editorConfig));
    memcpy(temp, conf, sizeof(struct editorConfig));
    atexit(term_exit);

    conf->termios.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    conf->termios.c_oflag &= ~(OPOST);
    conf->termios.c_iflag &= ~(IXON | ICRNL);
    conf->termios.c_cflag |= (CS8);
    conf->termios.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);

    // return 0 if nothing entered
    conf->termios.c_cc[VMIN] = 0;
    // return after 100 ms into output buffer
    conf->termios.c_cc[VTIME] = 1;

    if (tcsetattr(STDOUT_FILENO, 0, &conf->termios) == -1) {
        die("tcsetattr");
    }
}

void term_exit(void) {
    if (!temp || tcsetattr(STDIN_FILENO, TCSAFLUSH,
                           &((struct editorConfig*)temp)->termios) == -1) {
        die("tcsetattr");
    }
}

int term_get_window_size(struct editorConfig* conf) {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        return -1;
    } else {
        conf->screen_cols = ws.ws_col;
        conf->screen_rows = ws.ws_row;
        return 0;
    }
}
