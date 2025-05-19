#include <stdlib.h>

#include "inout.h"
#include "objects.h"
#include "terminal.h"

int main(void) {
    struct editorConfig *conf = malloc(sizeof(struct editorConfig));
    if (!conf) return -1;

    editor_create(conf);
    term_create(conf);
    while (1) {
        editor_refresh_screen(conf);
        editor_process_key_press();
    }

    free(conf);
    free(temp);

    return 0;
}