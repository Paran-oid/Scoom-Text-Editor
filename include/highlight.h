#ifndef HIGHLIGHT_H
#define HIGHLIGHT_H

#include <stdint.h>
struct Row;
struct EditorConfig;

enum EditorHighlight {
    HL_NORMAL = 0,
    HL_NUMBER,
    HL_MATCH,
    HL_STRING,
    HL_COMMENT,
    HL_MCOMMENT,
    HL_KEYWORD1,
    HL_KEYWORD2
};

struct EditorSyntax {
    char* filetype;
    char** filematch;
    char** keywords;
    char* singleline_comment_start;
    char* multiline_comment_start;
    char* multiline_comment_end;

    int8_t flags;

    char indent_start;
    char indent_end;
};

int8_t editor_syntax_highlight_select(struct EditorConfig* conf);
int8_t editor_syntax_to_color_row(enum EditorHighlight row);
int8_t editor_update_syntax(struct EditorConfig* conf, struct Row* row);

#endif
