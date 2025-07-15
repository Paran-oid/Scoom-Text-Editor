#include <stdlib.h>

#include "config.h"
#include "core.h"
#include "file.h"
#include "input.h"
#include "render.h"
#include "terminal.h"

int main(int argc, char* argv[]) {
    struct EditorConfig* conf = malloc(sizeof(struct EditorConfig));
    if (!conf) die("conf malloc failed");

    if (argc >= 2) {
        if (editor_open(conf, argv[1]) != EXIT_SUCCESS) {
            conf_destroy(conf);
            exit(EXIT_FAILURE);
        }
    }

    editor_run(conf);
	
    editor_destroy(conf);
    free(conf);

    exit(EXIT_SUCCESS);
}
