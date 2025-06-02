#include "highlight.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "core.h"
#include "objects.h"

#define HL_HIGHLIGHT_NUMBERS (1 << 0)

// init of database
char* C_HL_EXTENSIONS[] = {".c", ".h", ".cpp", NULL};

// HLDB: highlight database
struct EditorSyntax HLDB[] = {{"C", C_HL_EXTENSIONS, HL_HIGHLIGHT_NUMBERS}};

#define HLDB_ENTRIES (sizeof(HLDB) / sizeof(HLDB[0]))

int editor_syntax_highlight_select(struct Config* conf) {
    /*
            !strcmp(a, b) is equivalent to strcmp(a, b) == 0
    */

    conf->syntax = NULL;
    if (conf->filename == NULL) return 1;

    char* ext = strstr(conf->filename, ".");

    for (size_t i = 0; i < HLDB_ENTRIES; i++) {
        struct EditorSyntax* hl_entity = &HLDB[i];
        int j = 0;
        while (hl_entity->filematch[j]) {
            bool is_ext = hl_entity->filematch[j][0] == '.';
            if ((is_ext && ext && !strcmp(hl_entity->filematch[j], ext)) ||
                (!is_ext && !strcmp(hl_entity->filematch[j], conf->filename))) {
                conf->syntax = hl_entity;

                for (size_t filerow = 0; filerow < conf->numrows; filerow++) {
                    editor_update_syntax(conf, &conf->rows[filerow]);
                }
                return 0;
            }
            j++;
        }
    }

    return 1;
}

int editor_syntax_to_color_row(enum EditorHighlight hl) {
    switch (hl) {
        case HL_NUMBER:
            return 31;
        case HL_MATCH:
            return 34;
        default:
            return 37;
    }
}

int editor_update_syntax(struct Config* conf, struct e_row* row) {
    row->hl = realloc(row->hl, row->rsize);
    memset(row->hl, HL_NORMAL, row->rsize);

    if (!conf->syntax) return 1;

    size_t i = 0;
    bool prev_separator = false;
    while (i < row->size) {
        char c = row->render[i];
        unsigned char prev_hl = (i > 0) ? row->hl[i - 1] : HL_NORMAL;

        if (conf->syntax->flags & HL_HIGHLIGHT_NUMBERS) {
            if (isdigit(c) &&
                    ((prev_separator || (prev_hl == HL_NUMBER || i == 0))) ||
                (c == '.' && prev_hl == HL_NUMBER)) {
                row->hl[i] = HL_NUMBER;
                prev_separator = false;
            }
        }

        prev_separator = is_separator((unsigned char)c);
        i++;
    }

    return 0;
}
