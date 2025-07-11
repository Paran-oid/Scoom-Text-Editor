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
    "char|", "short|", "int|", "long|", "float|", "double|", "void|", "_Bool|",
    "unsigned|", "signed|", "size_t|", "ptrdiff_t|", "intptr_t|", "uintptr_t|",

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
    {"C", C_HL_EXTENSIONS, C_HL_KEYWORDS,
     HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS | HL_HIGHLIGHT_COMMENTS |
         HL_HIGHLIGHT_MCOMMENTS,
     "//", "/*", "*/", '{', '}'},

    {"Python", PY_HL_EXTENSIONS, PY_HL_KEYWORDS,
     HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS | HL_HIGHLIGHT_COMMENTS, "#",
     NULL, NULL, ':', '\0'},

    {"JavaScript", JS_HL_EXTENSIONS, JS_HL_KEYWORDS,
     HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS | HL_HIGHLIGHT_COMMENTS |
         HL_HIGHLIGHT_MCOMMENTS,
     "//", "/*", "*/", '{', '}'}};

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

int editor_syntax_highlight_select(struct EditorConfig* conf) {
    /*
     */

    conf->syntax = NULL;
    if (conf->filepath == NULL) return FILE_NOT_FOUND;

    char* filename;
    editor_extract_filename(conf, &filename);
    const char* ext = strrchr(filename, '.');

    for (size_t i = 0; i < HLDB_ENTRIES; i++) {
        struct EditorSyntax* hl_entity = &HLDB[i];
        int j = 0;
        while (hl_entity->filematch[j]) {
            bool is_ext = hl_entity->filematch[j][0] == '.';
            if ((is_ext && ext && strcmp(hl_entity->filematch[j], ext) == 0) ||
                (!is_ext &&
                 strcmp(hl_entity->filematch[j], conf->filepath) == 0)) {
                conf->syntax = hl_entity;

                for (int filerow = 0; filerow < conf->numrows; filerow++) {
                    editor_update_syntax(conf, &conf->rows[filerow]);
                }
                return SUCCESS;
            }
            j++;
        }
    }

    return SYNTAX_ERROR;
}

static size_t handle_multiline_comment(struct Row* row, size_t i,
                                       const char* mce, size_t mce_len,
                                       bool* in_comment) {
    row->hl[i] = HL_MCOMMENT;
    if (strncmp(&row->render[i], mce, mce_len) == 0) {
        memset(&row->hl[i], HL_MCOMMENT, mce_len);
        *in_comment = false;
        return mce_len;
    }
    return 1;
}

static size_t handle_string(struct Row* row, size_t i, int* in_string) {
    char c = row->render[i];
    row->hl[i] = HL_STRING;
    if (c == '\\' && i + 1 < row->size) {
        row->hl[i + 1] = HL_STRING;
        return 2;
    }
    if (c == *in_string) *in_string = 0;
    return 1;
}

static size_t handle_keywords(struct Row* row, size_t i, char** keywords) {
    for (size_t j = 0; keywords[j]; j++) {
        size_t klen = strlen(keywords[j]);
        int is_kw2 = keywords[j][klen - 1] == '|';
        if (is_kw2) klen--;
        if ((strncmp(&row->render[i], keywords[j], klen) == 0) &&
            check_seperator(row->render[i + klen])) {
            memset(&row->hl[i], is_kw2 ? HL_KEYWORD2 : HL_KEYWORD1, klen);
            return klen;
        }
    }
    return 0;
}

int editor_update_syntax(struct EditorConfig* conf, struct Row* row) {
    if (!row->chars) return NULL_PARAMETER;

    row->hl = realloc(row->hl, row->rsize);
    memset(row->hl, HL_NORMAL, row->rsize);

    if (!conf->syntax) return SYNTAX_ERROR;

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
                prev_separator = true;
                continue;
            } else if (strncmp(&row->render[i], mcs, mcs_len) == 0) {
                i += mcs_len;
                in_comment = true;
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
                prev_separator = false;
            }
        }

        // Keyword
        if (prev_separator) {
            size_t step = handle_keywords(row, i, keywords);
            if (step) {
                i += step;
                continue;
            }
        }

        prev_separator = check_seperator((unsigned char)c);
        i++;
    }

    int changed = (row->hl_open_comment != in_comment);
    row->hl_open_comment = in_comment;
    if (changed && row->idx + 1 < conf->numrows) {
        editor_update_syntax(conf, row + 1);
    }
    return SUCCESS;
}
