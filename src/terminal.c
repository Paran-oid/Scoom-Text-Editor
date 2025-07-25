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
#include "file.h"

// Any needed info can be found on termios.h
// TCSAFLUSH: flushes before leaving the program

struct termios orig_termios;

static void term_size_flag_update(int32_t sig) {
    (void)sig;
    if (g_conf) g_conf->flags.resize_needed = 1;
}

static void cleanup(void) {
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1)
        die("tcsetattr");

#if SCROLL_DISABLE
    // Show terminal scrollbar
    if (write(STDOUT_FILENO, "\x1b[?1049l", 9) == -1)
        die("couldn't show terminal scrollbar");

#endif

#if MOUSE_REPORTING
    if (write(STDOUT_FILENO, "\x1b[?1000l", 9) <= 0)
        die("couldn't disable mouse click");  // Disable mouse click
                                              // tracking

    if (write(STDOUT_FILENO, "\x1b[?1006l", 9) <= 0)
        die("couldn't disable SGR");  // Disable SGR mode

#endif
}

void term_create(void) {
    atexit(cleanup);

    struct termios raw;
    if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) die("tcgetattr");
    raw = orig_termios;

    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
        die("tcsetattr");
    }

#if SCROLL_DISABLE
    // Hide terminal's scrollbar
    if (write(STDOUT_FILENO, "\x1b[?1049h", 9) < 0) {
        die("couldn't hide terminal's scrollbar");
    }
#endif

    // signal(interrupt) handling
    struct sigaction sa;
    sa.sa_handler = term_size_flag_update;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0; /* <-- no SA_RESTART (By default, read() may restart
                     automatically after a signal if the system uses the
                    SA_RESTART flag. You need to disable this behavior.) */
                     // basically flag needed to handle any sort of interrupt

    sigaction(SIGWINCH, &sa, NULL);

    int32_t signals[] = {SIGSEGV, SIGINT, SIGTERM};
    for (int32_t i = 0; i < 3; ++i) {
        struct sigaction sa_exit;
        sa_exit.sa_handler = term_exit;
        sigemptyset(&sa_exit.sa_mask);
        sa_exit.sa_flags = 0;
        sigaction(signals[i], &sa_exit, NULL);
    }
#if MOUSE_REPORTING

    if (write(STDOUT_FILENO, "\x1b[?1000h", 9) < 0) {
        die("enabling mouse click error");
    }  // Enable mouse click tracking
    if (write(STDOUT_FILENO, "\x1b[?1006h", 9) < 0) {
        die("enabling SGR mode error");
    }  // Enable mouse click tracking
    // Enable SGR (1006) mode for xterm
#endif
}

void term_exit(int32_t sig __attribute__((unused))) {
    cleanup();
    exit(EXIT_SUCCESS);
}

int8_t term_get_window_size(struct EditorConfig* conf, int32_t* rows,
                            int32_t* cols) {
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
        return EXIT_SUCCESS;
    }
}

int8_t term_get_cursor_position(int32_t* rows, int32_t* cols) {
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

    return EXIT_SUCCESS;
}