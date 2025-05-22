#include "terminal.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include "inout.h"
#include "objects.h"

/*
        usage of each macro (BRKINT, ICRNL, INPCK, ...) can be understood in
        termios.h docs
*/

// TCSAFLUSH: flushes before leaving the program

void die(const char* s) {
    perror(s);
    exit(1);
}

void term_create(struct Config* conf) {
    if (tcgetattr(STDIN_FILENO, &conf->termios) == -1) {
        die("tcgetattr");
    }

    /*
        Since you can't pass any parameter for a function pointer in atexit,
       I decided to create a temp void* pointer in which I will pass the conf so
       that the terminal can be reset. Risky but possible
    */
    temp = malloc(sizeof(struct Config));
    memcpy(temp, conf, sizeof(struct Config));
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
                           &((struct Config*)temp)->termios) == -1) {
        die("tcsetattr");
    }
}

int term_get_window_size(struct Config* conf, int* rows, int* cols) {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        // idea:
        /*
                - Move cursor to max bottom and max right border
                - get cursor position and along update rows and cols
        */

        // C: cursor forward(continue)
        // B: cursor down(bottom)
        if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return -1;
        return term_get_cursor_position(conf, rows, cols);
    } else {
        conf->screen_cols = ws.ws_col;
        conf->screen_rows = ws.ws_row;
        return 0;
    }
}

int term_get_cursor_position(struct Config* conf, int* rows, int* cols) {
    size_t i = 0;
    char buf[32];
    // Command from host (\x1b[6n) – Please report active position (using a CPR
    // control sequence)
    if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;

    // result example: <esc>[rows;colsR
    while (i < sizeof(buf) - 1) {
        if (read(STDIN_FILENO, &buf[i], 1) != 1) break;
        if (buf[i] == 'R') break;
        i++;
    }
    buf[i] = '\0';
    if (buf[0] != '\x1b' || buf[1] != '[') return -1;
    if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) return -1;

    return 0;
}