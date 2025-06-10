#include <stdlib.h>

#include "config.h"
#include "file.h"
#include "input.h"
#include "render.h"
#include "terminal.h"

int main(int argc, char* argv[]) {	
    struct Config* conf = malloc(sizeof(struct Config));
    if (!conf) return EXIT_FAILURE;

    if (argc >= 2) {
        if (editor_open(conf, argv[1]) != 0) {
            config_destroy(conf);
            return EXIT_FAILURE;
        }
    }

    editor_run(conf);

    editor_destroy(conf);
    free(temp);

    return EXIT_SUCCESS;
}