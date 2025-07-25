#include "highlight.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "core.h"
#include "file.h"
#include "rows.h"

#define HL_HIGHLIGHT_NUMBERS (1 << 0)
#define HL_HIGHLIGHT_STRINGS (1 << 1)
#define HL_HIGHLIGHT_COMMENTS (1 << 2)
#define HL_HIGHLIGHT_MCOMMENTS (1 << 3)

// init of database
char* C_HL_EXTENSIONS[] = {".c", ".h", ".cpp", NULL};
char* C_HL_KEYWORDS[] = {
    // Control flow
    "if", "else", "switch", "case", "default", "for", "while", "do", "break",
    "continue", "return", "goto",

    // Data types (with | to highlight them as types)
    "char|", "short|", "int|", "long|", "float|", "double|", "void|",
    "_Bool|", "unsigned|", "signed|", "int32_t|", "ptrdiff_t|", "intptr_t|",
    "uintptr_t|",

    // Qualifiers & Storage class
    "const", "volatile", "static", "extern", "register", "auto", "restrict",
    "inline",

    // Type definitions & Structuring
    "struct", "union", "enum", "typedef", "sizeof|", "typeof|",

    // Preprocessor
    "#define|", "#undef|", "#include|", "#if|", "#ifdef|", "#ifndef|", "#else|",
    "#elif|", "#endif|", "#pragma|",

    // Boolean literals (C99+)
    "true|", "false|",

    // C++ compatibility (optional in case you handle C++)
    "class", "public", "private", "protected", "namespace", "new", "delete",
    "this", "operator", "try", "catch", "throw",

    NULL};

char* PY_HL_EXTENSIONS[] = {".py", NULL};
char* PY_HL_KEYWORDS[] = {"def",    "return",   "if",     "elif",     "else",
                          "for",    "while",    "break",  "continue", "pass",
                          "import", "from",     "as",     "class",    "try",
                          "except", "finally",  "raise",  "with",     "lambda",
                          "global", "nonlocal", "assert", "yield",    "del",
                          "True|",  "False|",   "None|",  NULL};

char* JS_HL_EXTENSIONS[] = {".js", ".jsx", NULL};
char* JS_HL_KEYWORDS[] = {
    "function", "return",   "if",     "else",   "for",     "while",
    "break",    "continue", "var",    "let",    "const",   "switch",
    "case",     "default",  "try",    "catch",  "finally", "throw",
    "class",    "extends",  "import", "from",   "export",  "new",
    "this",     "super",    "true|",  "false|", "null|",   "undefined|",
    NULL};

// HLDB: highlight database

struct EditorSyntax HLDB[] = {
    {"C", C_HL_EXTENSIONS, C_HL_KEYWORDS, "//", "/*", "*/",
     HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS | HL_HIGHLIGHT_COMMENTS |
         HL_HIGHLIGHT_MCOMMENTS,
     '{', '}'},

    {"Python", PY_HL_EXTENSIONS, PY_HL_KEYWORDS, "#", NULL, NULL,
     HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS | HL_HIGHLIGHT_COMMENTS, ':',
     '\0'},

    {"JavaScript", JS_HL_EXTENSIONS, JS_HL_KEYWORDS, "//", "/*", "*/",
     HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS | HL_HIGHLIGHT_COMMENTS |
         HL_HIGHLIGHT_MCOMMENTS,
     '{', '}'}};

#define HLDB_ENTRIES (sizeof(HLDB) / sizeof(HLDB[0]))

int8_t editor_syntax_to_color_row(enum EditorHighlight hl) {
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

int8_t editor_syntax_highlight_select(struct EditorConfig* conf) {
    conf->syntax = NULL;

    if (!conf->filepath) return EXIT_FAILURE;

    char* filename;
    if (editor_extract_filename(conf, &filename))
        die("extracting filename failed");
    const char* ext = strrchr(filename, '.');

    for (size_t i = 0; i < HLDB_ENTRIES; i++) {
        struct EditorSyntax* hl_entity = &HLDB[i];
        size_t j = 0;
        while (hl_entity->filematch[j]) {
            int8_t is_ext = hl_entity->filematch[j][0] == '.';
            if ((is_ext && ext && strcmp(hl_entity->filematch[j], ext) == 0) ||
                (!is_ext &&
                 strcmp(hl_entity->filematch[j], conf->filepath) == 0)) {
                conf->syntax = hl_entity;

                for (int32_t filerow = 0; filerow < conf->numrows; filerow++) {
                    editor_update_syntax(conf, &conf->rows[filerow]);
                }
                return EXIT_SUCCESS;
            }
            j++;
        }
    }

    return EXIT_FAILURE;
}

/*
    ? in the upcoming static functions we return numbers because each of these
    ? returns return how much to step
*/

static int32_t handle_multiline_comment(struct Row* row, int32_t i,
                                        const char* mce, int32_t mce_len,
                                        int8_t* in_comment) {
    row->hl[i] = HL_MCOMMENT;
    if (strncmp(&row->render[i], mce, mce_len) == 0) {
        memset(&row->hl[i], HL_MCOMMENT, mce_len);
        *in_comment = 0;
        return mce_len;
    }
    return 1;
}

static int32_t handle_string(struct Row* row, int32_t i, int8_t* in_string) {
    char c = row->render[i];
    row->hl[i] = HL_STRING;
    if (c == '\\' && i + 1 < row->size) {
        row->hl[i + 1] = HL_STRING;
        return 2;
    }
    if (c == *in_string) *in_string = 0;
    return 1;
}

static int32_t handle_keywords(struct Row* row, int32_t i, char** keywords) {
    for (int32_t j = 0; keywords[j]; j++) {
        int32_t klen = strlen(keywords[j]);
        int8_t is_kw2 = keywords[j][klen - 1] == '|';
        if (is_kw2) klen--;
        if ((strncmp(&row->render[i], keywords[j], klen) == 0) &&
            check_seperator(row->render[i + klen])) {
            memset(&row->hl[i], is_kw2 ? HL_KEYWORD2 : HL_KEYWORD1, klen);
            return klen;
        }
    }
    return 0;
}

int8_t editor_update_syntax(struct EditorConfig* conf, struct Row* row) {
    if (!row->chars) return EXIT_FAILURE;

    row->hl = realloc(row->hl, row->rsize);
    memset(row->hl, HL_NORMAL, row->rsize);

    if (!conf->syntax) return EXIT_FAILURE;

    char** keywords = conf->syntax->keywords;

    char* scs = conf->syntax->singleline_comment_start;
    char* mcs = conf->syntax->multiline_comment_start;
    char* mce = conf->syntax->multiline_comment_end;

    int32_t scs_len = scs ? strlen(scs) : 0;
    int32_t mcs_len = mcs ? strlen(mcs) : 0;
    int32_t mce_len = mcs ? strlen(mce) : 0;

    int32_t i = 0;
    int8_t prev_separator = 1;
    int8_t in_comment =
        (row->idx > 0 && conf->rows[row->idx - 1].hl_open_comment);

    int8_t in_string = 0;

    while (i < row->rsize) {
        char c = row->render[i];
        unsigned char prev_hl = (i > 0) ? row->hl[i - 1] : HL_NORMAL;

        // Singleline comment
        if (scs_len && !in_string && !in_comment) {
            if (i + scs_len <= row->size &&
                strncmp(&row->chars[i], scs, scs_len) == 0) {
                memset(&row->hl[i], HL_COMMENT, row->size - i);
                break;
            }
        }

        // Multi-line comment
        if (mcs_len && mce_len && !in_string) {
            if (in_comment) {
                i +=
                    handle_multiline_comment(row, i, mce, mce_len, &in_comment);
                prev_separator = 1;
                continue;
            } else if (strncmp(&row->render[i], mcs, mcs_len) == 0) {
                i += mcs_len;
                in_comment = 1;
                continue;
            }
        }

        // String
        if (conf->syntax->flags & HL_HIGHLIGHT_STRINGS) {
            if (in_string) {
                i += handle_string(row, i, &in_string);
                prev_separator = 1;
                continue;
            } else if (c == '"' || c == '\'' || c == '\"') {
                in_string = c;
                row->hl[i++] = HL_STRING;
                continue;
            }
        }

        // Number
        if (conf->syntax->flags & HL_HIGHLIGHT_NUMBERS) {
            if ((isdigit(c) && (prev_separator || prev_hl == HL_NUMBER)) ||
                (c == '.' && prev_hl == HL_NUMBER)) {
                row->hl[i] = HL_NUMBER;
                prev_separator = 0;
            }
        }

        // Keyword
        if (prev_separator) {
            int32_t step = handle_keywords(row, i, keywords);
            if (step) {
                i += step;
                continue;
            }
        }

        prev_separator = check_seperator((unsigned char)c);
        i++;
    }

    int32_t changed = (row->hl_open_comment != in_comment);
    row->hl_open_comment = in_comment;
    if (changed && row->idx + 1 < conf->numrows) {
        editor_update_syntax(conf, row + 1);
    }
    return EXIT_SUCCESS;
}
