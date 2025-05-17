#include "inout.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "terminal.h"

char editor_read_key(void) {
    char c;
    int nread;
    while ((nread = read(STDIN_FILENO, &c, 1)) != -1) {
        if (nread == -1 && errno != EAGAIN) die("read");
    }
    return c;
}
void editor_process_key_press(void) {
    char c = editor_read_key();
    switch (c) {
        case CTRL_KEY('q'):
            exit(0);
            break;
        default:
            printf("%c\r\n", c);
    }
}