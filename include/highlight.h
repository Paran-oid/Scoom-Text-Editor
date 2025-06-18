#ifndef HIGHLIGHT_H
#define HIGHLIGHT_H

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
    int flags;
    char* singleline_comment_start;
    char* multiline_comment_start;
    char* multiline_comment_end;

    char indent;
};

int editor_syntax_highlight_select(struct EditorConfig* conf);
int editor_syntax_to_color_row(enum EditorHighlight row);
int editor_update_syntax(struct EditorConfig* conf, struct Row* row);

#endif
