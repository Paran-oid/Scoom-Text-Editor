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

    char* ptest = "holy_crap.txt";
    if (argc >= 2) {
        editor_open(conf, argv[1]);
    } else if (ptest) {
        editor_open(conf, ptest);
    }

    while (1) {
        editor_refresh_screen(conf);
        editor_process_key_press(conf);
    }

    free(conf);
    free(temp);

    return 0;
}