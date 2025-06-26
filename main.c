#include <stdlib.h>

#include "config.h"
#include "file.h"
#include "input.h"
#include "render.h"
#include "terminal.h"

int main(int argc, char* argv[]) {
    struct EditorConfig* conf = malloc(sizeof(struct EditorConfig));
    if (!conf) return OUT_OF_MEMORY;
    if (argc >= 2) {
        if (editor_open(conf, argv[1]) != SUCCESS) {
            conf_destroy(conf);
            return INVALID_ARG;
        }
    }

    editor_run(conf);
    editor_destroy(conf);
    free(conf);
    g_conf = NULL;

    exit(SUCCESS);
}