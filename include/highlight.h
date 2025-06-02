#ifndef HIGHLIGHT_H
#define HIGHLIGHT_H

enum EditorHighlight;
struct e_row;
struct Config;

int editor_syntax_highlight_select(struct Config* conf);
int editor_syntax_to_color_row(enum EditorHighlight row);
int editor_update_syntax(struct Config* conf, struct e_row* row);

#endif
