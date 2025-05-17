#include "terminal.h"

#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

static struct termios term;

// usage of each macro can be understood in termios.h docs

// TCSAFLUSH: flushes before leaving the program

void die(const char* s) {
    perror(s);
    exit(1);
}

void term_create(void) {
    if (tcgetattr(STDIN_FILENO, &term) == -1) {
        die("tcgetattr");
    }
    atexit(term_exit);

    term.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    term.c_oflag &= ~(OPOST);
    term.c_iflag &= ~(IXON | ICRNL);
    term.c_cflag |= (CS8);
    term.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);

    // return 0 if nothing entered
    term.c_cc[VMIN] = 0;
    // return after 100 ms into output buffer
    term.c_cc[VTIME] = 1;

    if (tcsetattr(STDOUT_FILENO, 0, &term) == -1) {
        die("tcsetattr");
    }
}

void term_exit(void) {
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &term) == -1) {
        die("tcsetattr");
    }
}
