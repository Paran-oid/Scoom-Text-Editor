#include <stdlib.h>

#include "config.h"
#include "file.h"
#include "input.h"
#include "render.h"
#include "terminal.h"

#define DEBUG

int main(int argc, char* argv[]) {
    struct Config* conf = malloc(sizeof(struct Config));

    if (!conf) return EXIT_FAILURE;

    config_create(conf);
    term_create(conf);

    if (argc >= 2) {
        if (editor_open(conf, argv[1]) != 0) {
            config_destroy(conf);
            return EXIT_FAILURE;
        }
    }
#ifdef DEBUG
    else {
        if (editor_open(conf, "test.c") != 0) {
            config_destroy(conf);
            return EXIT_FAILURE;
        }
    }
#endif

    editor_set_status_message(
        conf, "HELP: CTRL-S = save | CTRL-Q = Quit | CTRL-F = Find");

    while (1) {
        editor_refresh_screen(conf);
        editor_process_key_press(conf);
    }

    config_destroy(conf);
    free(temp);

    return 0;
}