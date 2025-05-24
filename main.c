#include <stdlib.h>

#include "file.h"
#include "inout.h"
#include "objects.h"
#include "terminal.h"

int main(int argc, char* argv[]) {
    struct Config* conf = malloc(sizeof(struct Config));

    if (!conf) return -1;

    editor_create(conf);
    term_create(conf);

    if (argc >= 2) {
        editor_open(conf, argv[1]);
    }

    editor_set_status_message(conf, "HELP: CTRL-Q = Quit");

    while (1) {
        editor_refresh_screen(conf);
        editor_process_key_press(conf);
    }

    free(conf);
    free(temp);

    // TODO: organize structure of work and add functions to their respective
    // files...
    return 0;
}