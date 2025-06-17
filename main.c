#include <stdlib.h>

#include "config.h"
#include "file.h"
#include "input.h"
#include "render.h"
#include "terminal.h"

int main(int argc, char* argv[]) {
    struct EditorConfig* conf = malloc(sizeof(struct EditorConfig));
    if (!conf) return EXIT_FAILURE;

    if (argc >= 2) {
        if (editor_open(conf, argv[1]) != 0) {
            conf_destroy(conf);
            return EXIT_FAILURE;
        }
    }

    editor_run(conf);

    editor_destroy(conf);
    free(temp);
    free(conf);

    return EXIT_SUCCESS;
}