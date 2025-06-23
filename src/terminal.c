#include "terminal.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include "config.h"
#include "core.h"

/*
        usage of each macro (BRKINT, ICRNL, INPCK, ...) can be understood in
        termios.h docs
*/

// TCSAFLUSH: flushes before leaving the program

static void term_size_flag_update(int sig) {
    (void)sig;
    if (g_conf) {
        g_conf->resize_needed = 1;
    }
}

void term_create(struct EditorConfig* conf) {
    if (tcgetattr(STDIN_FILENO, &conf->orig_termios) == -1) {
        die("tcgetattr");
    }

    /*
        Since you can't pass any parameter for a function pointer in atexit,
       I decided to create a g_conf void* pointer in which I will pass the
       conf so that the terminal can be reset. Risky but possible
    */
    g_conf = malloc(sizeof(struct EditorConfig));
    memcpy(g_conf, conf, sizeof(struct EditorConfig));
    atexit(term_exit);

    conf->orig_termios.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    conf->orig_termios.c_oflag &= ~(OPOST);
    conf->orig_termios.c_iflag &= ~(IXON | ICRNL);
    conf->orig_termios.c_cflag |= (CS8);
    conf->orig_termios.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);

    // return 0 if nothing entered
    conf->orig_termios.c_cc[VMIN] = 0;
    // return after 100 ms into output buffer
    conf->orig_termios.c_cc[VTIME] = 1;

    // Hide terminal's scrollbar
    // TODO: reenable
    // if (write(STDOUT_FILENO, "\x1b[?1049h", 9) == -1) {
    //     die("couldn't hide terminal's scrollbar");
    // }

    // signal(interrupt) handling
    struct sigaction sa;
    sa.sa_handler = term_size_flag_update;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0; /* <-- no SA_RESTART (By default, read() may restart
                     automatically after a signal if the system uses the
                    SA_RESTART flag. You need to disable this behavior.) */
    sigaction(SIGWINCH, &sa, NULL);

    if (tcsetattr(STDOUT_FILENO, 0, &conf->orig_termios) == -1) {
        die("tcsetattr");
    }
}

void term_exit(void) {
    if (!g_conf ||
        tcsetattr(STDIN_FILENO, TCSAFLUSH,
                  &((struct EditorConfig*)g_conf)->orig_termios) == -1) {
        die("tcsetattr");
    }
    // TODO: disable
    // Show terminal scrollbar
    // if (write(STDOUT_FILENO, "\x1b[?1049l", 9) == -1)
    //     die("couldn't show terminal scrollbar");
}

int term_get_window_size(struct EditorConfig* conf, int* rows, int* cols) {
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
        return term_get_cursor_position(rows, cols);
    } else {
        conf->screen_cols = ws.ws_col;
        conf->screen_rows = ws.ws_row;
        return SUCCESS;
    }
}

int term_get_cursor_position(int* rows, int* cols) {
    size_t i = 0;
    char buf[32];
    // Command from host (\x1b[6n) – Please report active position (using a
    // CPR control sequence)
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

    return SUCCESS;
}