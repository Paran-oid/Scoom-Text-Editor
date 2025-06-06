#include <stdlib.h>

#include "file.h"
#include "inout.h"
#include "objects.h"
#include "terminal.h"

// #define DEBUG

int main(int argc, char* argv[]) {
    struct Config* conf = malloc(sizeof(struct Config));

    if (!conf) return -1;

    editor_create(conf);
    term_create(conf);

#ifdef DEBUG
    char* ptest = "holy_crap.c";
    editor_open(conf, ptest);

#endif
    if (argc >= 2) {
        editor_open(conf, argv[1]);
    }

    editor_set_status_message(
        conf, "HELP: CTRL-S = save | CTRL-Q = Quit | CTRL-F = Find");

    while (1) {
        editor_refresh_screen(conf);
        editor_process_key_press(conf);
    }

    editor_destroy(conf);
    free(temp);

    return 0;
}