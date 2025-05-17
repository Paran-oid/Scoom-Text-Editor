#include "inout.h"
#include "terminal.h"

int main(void) {
    term_create();

    while (1) {
        editor_process_key_press();
    }

    return 0;
}