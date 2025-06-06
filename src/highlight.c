#include "highlight.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "core.h"
#include "objects.h"

#define HL_HIGHLIGHT_NUMBERS (1 << 0)
#define HL_HIGHLIGHT_STRINGS (1 << 1)
#define HL_HIGHLIGHT_COMMENTS (1 << 2)
#define HL_HIGHLIGHT_MCOMMENTS (1 << 3)

// init of database
char* C_HL_EXTENSIONS[] = {".c", ".h", ".cpp", NULL};
char* C_HL_KEYWORDS[] = {"switch",    "if",      "while",   "for",    "break",
                         "continue",  "return",  "else",    "struct", "union",
                         "typedef",   "static",  "enum",    "class",  "case",
                         "int|",      "long|",   "double|", "float|", "char|",
                         "unsigned|", "signed|", "void|",   NULL};

// HLDB: highlight database
struct EditorSyntax HLDB[] = {{"C", C_HL_EXTENSIONS, C_HL_KEYWORDS,
                               HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS |
                                   HL_HIGHLIGHT_COMMENTS |
                                   HL_HIGHLIGHT_MCOMMENTS,
                               "//", "/*", "*/"}};

#define HLDB_ENTRIES (sizeof(HLDB) / sizeof(HLDB[0]))

int editor_syntax_to_color_row(enum EditorHighlight hl) {
    /*
            Colors are ranged from 31 to 37
            39 is to reset color to it's default
    */
    switch (hl) {
        case HL_NUMBER:
            return 31;
        case HL_MATCH:
            return 34;
        case HL_STRING:
            return 35;
        case HL_COMMENT:
            return 36;
        case HL_MCOMMENT:
            return 36;
        case HL_KEYWORD1:
            return 33;
        case HL_KEYWORD2:
            return 32;
        default:
            return 37;
    }
}

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

                for (int filerow = 0; filerow < conf->numrows; filerow++) {
                    editor_update_syntax(conf, &conf->rows[filerow]);
                }
                return 0;
            }
            j++;
        }
    }

    return 1;
}

int editor_update_syntax(struct Config* conf, struct e_row* row) {
    row->hl = realloc(row->hl, row->rsize);
    memset(row->hl, HL_NORMAL, row->rsize);

    if (!conf->syntax) return 1;

    char** keywords = conf->syntax->keywords;

    char* scs = conf->syntax->singleline_comment_start;
    char* mcs = conf->syntax->multiline_comment_start;
    char* mce = conf->syntax->multiline_comment_end;

    size_t scs_len = scs ? strlen(scs) : 0;
    size_t mcs_len = mcs ? strlen(mcs) : 0;
    size_t mce_len = mcs ? strlen(mce) : 0;

    size_t i = 0;
    bool prev_separator = true;
    bool in_comment =
        (row->idx > 0 && conf->rows[row->idx - 1].hl_open_comment);

    int in_string = 0;

    while (i < row->size) {
        char c = row->render[i];
        unsigned char prev_hl = (i > 0) ? row->hl[i - 1] : HL_NORMAL;

        if (scs_len && !in_string && !in_comment) {
            // if we have a comment
            if (strncmp(&row->chars[i], scs, scs_len) == 0) {
                memset(&row->hl[i], HL_COMMENT, row->size - i);
                break;
            }
        }

        if (mcs_len && mce_len && !in_string) {
            if (in_comment) {
                row->hl[i] = HL_MCOMMENT;
                if (strncmp(&row->render[i], mce, mce_len) == 0) {
                    memset(&row->hl[i], HL_MCOMMENT, mce_len);
                    i += mce_len;
                    in_comment = false;
                    prev_separator = true;
                    continue;
                } else {
                    i++;
                    continue;
                }
            } else if (strncmp(&row->render[i], mcs, mcs_len) == 0) {
                i += mcs_len;
                in_comment = true;
                continue;
            }
        }

        if (conf->syntax->flags & HL_HIGHLIGHT_STRINGS) {
            if (in_string) {
                row->hl[i] = HL_STRING;
                if (c == '\\' && i < row->size) {
                    row->hl[i + 1] = HL_STRING;
                    i += 2;
                    continue;
                }
                if (c == in_string) in_string = 0;
                i++;
                prev_separator = 1;
                continue;
            } else {
                if (c == '"' || c == '\'') {
                    in_string = c;
                    row->hl[i] = HL_STRING;
                    i++;
                    continue;
                }
            }
        }

        if (conf->syntax->flags & HL_HIGHLIGHT_NUMBERS) {
            if ((isdigit(c) && (prev_separator || prev_hl == HL_NUMBER)) ||
                (c == '.' && prev_hl == HL_NUMBER)) {
                row->hl[i] = HL_NUMBER;
                prev_separator = false;
            }
        }

        if (prev_separator) {
            for (size_t j = 0; keywords[j]; j++) {
                size_t klen = strlen(keywords[j]);
                int is_kw2 = keywords[j][klen - 1] == '|';

                if (is_kw2) klen--;

                if ((strncmp(&row->render[i], keywords[j], klen) == 0) &&
                    is_separator(
                        row->render[i + klen])  // we added section condition
                                                // cuz \0 is also a separator
                                                // technically

                ) {
                    memset(&row->hl[i], is_kw2 ? HL_KEYWORD2 : HL_KEYWORD1,
                           klen);
                    i += klen;
                    break;
                }
            }
        }

        prev_separator = is_separator((unsigned char)c);
        i++;
    }

    int changed = (row->hl_open_comment != in_comment);
    row->hl_open_comment = in_comment;
    if (changed && row->idx + 1 < conf->numrows) {
        editor_update_syntax(conf, row + 1);
    }
    return 0;
}