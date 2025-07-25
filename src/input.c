#include "input.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "config.h"
#include "core.h"
#include "file.h"
#include "render.h"
#include "rows.h"
#include "terminal.h"

static void skip_word_forward(struct EditorConfig *conf, struct Row *row,
                              int32_t numline_offset) {
    int32_t cursor_offset = conf->cx - numline_offset;
    while (conf->cx < row->size + numline_offset &&
           !ISCHAR(row->chars[cursor_offset]) &&
           (row->chars[cursor_offset] != '_'))
        conf->cx++;
    while (conf->cx < row->size + numline_offset &&
           (ISCHAR(row->chars[cursor_offset]) ||
            (row->chars[cursor_offset] == '_')))
        conf->cx++;
}

static void skip_word_backward(struct EditorConfig *conf, struct Row *row,
                               int32_t numline_offset) {
    if (conf->cx == numline_offset) return;

    if (!conf->cx) conf->cx--;

    int32_t cursor_offset = conf->cx - numline_offset;

    while (conf->cx > numline_offset && !ISCHAR(row->chars[cursor_offset]) &&
           (row->chars[cursor_offset] != '_'))
        if (!conf->cx) conf->cx--;

    while (conf->cx > numline_offset && (ISCHAR(row->chars[cursor_offset]) ||
                                         (row->chars[cursor_offset] == '_')))
        if (!conf->cx) conf->cx--;

    if (!ISCHAR(row->chars[cursor_offset]) &&
        (row->chars[cursor_offset] != '_')) {
        conf->cx++;
    }
}

int8_t editor_cursor_ctrl(struct EditorConfig *conf, int32_t key) {
    if (conf->cy < 0 || conf->cy >= conf->numrows) return EXIT_FAILURE;
    struct Row *row = &conf->rows[conf->cy];
    int32_t numline_offset = editor_row_numline_calculate(row);

    if (key == CTRL_ARROW_RIGHT) {
        if (conf->cx == row->size + numline_offset) {
            conf->cy++;
            if (conf->cy >= conf->numrows - 1) {
                conf->cy--;
                return EXIT_FAILURE;
            }
            row = &conf->rows[conf->cy];
            conf->cx = numline_offset;
        }
        skip_word_forward(conf, row, numline_offset);
    } else {
        if (conf->cx == numline_offset) {
            conf->cy--;
            if (conf->cy < 0) {
                conf->cy = 0;
                return EXIT_FAILURE;
            }
            row = &conf->rows[conf->cy];
            conf->cx = row->size != 0 ? row->size - 1 : numline_offset;
        }
        skip_word_backward(conf, row, numline_offset);
    }
    return EXIT_SUCCESS;
}

int8_t editor_cursor_move(struct EditorConfig *conf, int32_t key) {
    struct Row *row =
        (conf->cy >= conf->numrows) ? NULL : &conf->rows[conf->cy];

    int32_t numline_offset = 0;
    if (row) {
        numline_offset = editor_row_numline_calculate(row);
    }

    int32_t desired_cx_logical = conf->cx - numline_offset;

    switch (key) {
        case ARROW_LEFT:
            if (conf->cx != numline_offset) {
                if (conf->coloff != 0 &&
                    conf->cx == numline_offset + conf->coloff) {
                    conf->coloff--;
                }
                if (!conf->cx) conf->cx--;

            } else if (conf->cy > 0) {
                conf->cy--;
                row =
                    (conf->cy >= conf->numrows) ? NULL : &conf->rows[conf->cy];
                numline_offset = editor_row_numline_calculate(row);
                conf->cx = row->size + numline_offset;
            }
            break;
        case ARROW_RIGHT:
            if (row && conf->cx < row->size + numline_offset) {
                conf->cx++;
            } else if (row && conf->cy < conf->numrows - 1) {
                row = &conf->rows[++conf->cy];
                numline_offset =
                    editor_row_numline_calculate(row);  // recalculate it
                conf->cx = numline_offset;
            }
            break;
        case ARROW_UP:
            if (conf->cy > 0) {
                conf->cy--;
                row = &conf->rows[conf->cy];
                numline_offset = editor_row_numline_calculate(row);
                if (desired_cx_logical > row->size)
                    desired_cx_logical = row->size;
                conf->cx = numline_offset + desired_cx_logical;
            }
            break;
        case ARROW_DOWN:
            if (conf->cy < conf->numrows - 1) {
                conf->cy++;
                row = &conf->rows[conf->cy];
                numline_offset = editor_row_numline_calculate(row);
                if (desired_cx_logical > row->size)
                    desired_cx_logical = row->size;
                conf->cx = numline_offset + desired_cx_logical;
            }
            break;
        default:
            die("invalid input...");
    }
    row = (conf->cy >= conf->numrows) ? NULL : &conf->rows[conf->cy];
    if (row && conf->cx > row->size + numline_offset) {
        conf->cx = row->size;
    }

    conf->rx =
        editor_update_cx_rx(row, conf->cx - numline_offset) + numline_offset;

    return EXIT_SUCCESS;
}

int8_t editor_shift_select(struct EditorConfig *conf, int32_t key) {
    struct Row *row = &conf->rows[conf->cy];
    int32_t numline_offset = editor_row_numline_calculate(row);
    struct EditorCursorSelect *sel = &conf->sel;

    // basically taking into account numline offset
    const int32_t real_rx = conf->rx - numline_offset;

    if (sel->start_col == -1 || sel->end_col == -1 || sel->start_row == -1 ||
        sel->end_row == -1) {
        conf_select_update(conf, conf->cy, conf->cy, conf->cx, conf->cx);
    }

    switch (key) {
        case SHIFT_ARROW_LEFT:
            // at beginning of first line
            if (conf->cy <= 0 && conf->rx == numline_offset) break;
            editor_cursor_move(conf, ARROW_LEFT);

            if (!sel->active || conf_check_cursor_anchor(conf, sel->start_row,
                                                         sel->start_col) ==
                                    CURSOR_ANCHOR_BEFORE) {
                conf_select_update(conf, conf->cy, sel->end_row, conf->rx,
                                   sel->end_col);
            } else {
                conf_select_update(conf, sel->start_row, conf->cy,
                                   sel->start_col, conf->rx);
            }

            break;

        case SHIFT_ARROW_UP:
            // at the first line
            if (conf->cy == conf->rowoff) break;
            editor_cursor_move(conf, ARROW_UP);

            if (!sel->active || conf_check_cursor_anchor(conf, sel->start_row,
                                                         sel->start_col) ==
                                    CURSOR_ANCHOR_BEFORE) {
                conf_select_update(conf, conf->cy, sel->end_row, conf->rx,
                                   sel->end_col);
            } else {
                conf_select_update(conf, sel->start_row, conf->cy,
                                   sel->start_col, conf->rx);
            }

            break;

        case SHIFT_ARROW_RIGHT:
            // at end of last line
            if (conf->cy >= conf->numrows - 1 && real_rx == row->rsize) break;
            editor_cursor_move(conf, ARROW_RIGHT);

            // if we are not selecting and didn't already move the cursor
            if (!sel->active ||
                conf_check_cursor_anchor(conf, sel->end_row, sel->end_col) ==
                    CURSOR_ANCHOR_AFTER) {
                conf_select_update(conf, sel->start_row, conf->cy,
                                   sel->start_col, conf->rx);
            } else {
                conf_select_update(conf, conf->cy, sel->end_row, conf->rx,
                                   sel->end_col);
            }

            break;

        case SHIFT_ARROW_DOWN:
            // at last line
            if (conf->cy == conf->numrows - 1) break;
            editor_cursor_move(conf, ARROW_DOWN);
            if (!sel->active ||
                conf_check_cursor_anchor(conf, sel->end_row, sel->end_col) ==
                    CURSOR_ANCHOR_AFTER) {
                conf_select_update(conf, sel->start_row, conf->cy,
                                   sel->start_col, conf->rx);
            } else {
                conf_select_update(conf, conf->cy, sel->end_row, conf->rx,
                                   sel->end_col);
            }
            break;
    }

    if (sel->start_row == sel->end_row && sel->start_col == sel->end_col) {
        sel->active = 0;
    } else {
        sel->active = 1;
    }
    return EXIT_SUCCESS;
}

int32_t editor_read_key(struct EditorConfig *conf) {
    int32_t c;
    int32_t nread;
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if (conf->flags.resize_needed) return INTERRUPT_ENCOUNTERED;

        if (nread == -1 && errno != EAGAIN && errno != EINTR) {
            die("editor failed to read key to input");
        }
    }

    if (c == '\x1b') {
        char seq[32];

        if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
        if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';
        if (seq[0] == '[') {
            if ('0' <= seq[1] && seq[1] <= '9') {
                if (read(STDIN_FILENO, &seq[2], 1) != 1) return '\x1b';
                if (seq[2] == '~') {
                    switch (seq[1]) {
                        case '1':
                        case '7':
                            return HOME_KEY;
                        case '3':
                            return DEL_KEY;
                        case '4':
                        case '8':
                            return END_KEY;
                        case '5':
                            return PAGE_UP;
                        case '6':
                            return PAGE_DOWN;
                    }
                } else if (seq[2] == ';') {
                    if (read(STDIN_FILENO, &seq[3], 1) != 1) return '\x1b';
                    if (read(STDIN_FILENO, &seq[4], 1) != 1) return '\x1b';
                    if (seq[1] == '1' && seq[3] == '5') {
                        switch (seq[4]) {
                            case 'A':
                                return CTRL_ARROW_UP;
                                break;
                            case 'B':
                                return CTRL_ARROW_DOWN;
                                break;
                            case 'C':
                                return CTRL_ARROW_RIGHT;
                                break;
                            case 'D':
                                return CTRL_ARROW_LEFT;
                                break;
                        }
                    } else if (seq[1] == '1' && seq[3] == '2') {
                        switch (seq[4]) {
                            case 'A':
                                return SHIFT_ARROW_UP;
                            case 'B':
                                return SHIFT_ARROW_DOWN;
                            case 'C':
                                return SHIFT_ARROW_RIGHT;
                            case 'D':
                                return SHIFT_ARROW_LEFT;
                        }
                    }
                }
            }
            switch (seq[1]) {
                case 'A':
                    return ARROW_UP;
                case 'B':
                    return ARROW_DOWN;
                case 'C':
                    return ARROW_RIGHT;
                case 'D':
                    return ARROW_LEFT;
                case 'H':
                    return HOME_KEY;
                case 'F':
                    return END_KEY;
            }
        } else if (seq[0] == 'O') {
            switch (seq[1]) {
                case 'H':
                    return HOME_KEY;
                    break;
                case 'F':
                    return END_KEY;
                    break;
            }
        } else {
            return '\x1b';
        }
    } else {
        return c;
    }

    return EXIT_FAILURE;
}

/*
        Side Note:
        We use int32_t c instead of enum EditorKey c since we have mapped
        some keys to values bigger than the max 255 like for ARROWS
*/
int8_t editor_process_key_press(struct EditorConfig *conf) {
    static int8_t quit_times = QUIT_TIMES;

    struct Snapshot *s = NULL;
    struct Row *row =
        (conf->cy >= conf->numrows) ? NULL : &conf->rows[conf->cy];
    int32_t c = editor_read_key(conf);

    // times var will be needed to page up or down
    int8_t times = conf->screen_rows;

    time_t current_time = time(NULL);
    int64_t time_elapsed = difftime(current_time, conf->last_time_modified);
    conf->sel.active = 0;

    switch (c) {
        case F1:
        case F2:
        case F3:
        case F4:
        case F5:
        case F6:
        case F7:
        case F8:
        case F9:
        case F10:
        case F11:
        case F12:
            break;

            // TODO: do something about this commented code

        case INTERRUPT_ENCOUNTERED:
            conf->flags.resize_needed = 0;
            term_get_window_size(conf, &conf->screen_rows, &conf->screen_cols);
            conf->screen_rows -= 2;  // for prompt and message rows
            break;
        case '\r':
            s = malloc(sizeof(struct Snapshot));
            snapshot_create(conf, s);
            stack_push(conf->stack_undo, s);

            editor_insert_newline(conf);
            break;

        case ARROW_UP:
        case ARROW_DOWN:
        case ARROW_RIGHT:
        case ARROW_LEFT:

            editor_cursor_move(conf, c);
            break;

        case SHIFT_ARROW_UP:
        case SHIFT_ARROW_DOWN:
        case SHIFT_ARROW_RIGHT:
        case SHIFT_ARROW_LEFT:
            conf->sel.active = 1;
            editor_shift_select(conf, c);
            break;

        case CTRL_ARROW_UP:
            if (conf->rowoff > 0) {
                conf->cy--;
                conf->rowoff--;
            }
            break;

        case CTRL_ARROW_DOWN:
            if (conf->rowoff < conf->numrows - 6) {
                conf->rowoff++;
                conf->cy++;
            }
            break;

        case CTRL_ARROW_LEFT:
        case CTRL_ARROW_RIGHT:
            editor_cursor_ctrl(conf, c);
            break;

        case PAGE_UP:
        case PAGE_DOWN:
            if (c == PAGE_UP) {
                conf->cy = conf->rowoff;
            } else if (c == PAGE_DOWN) {
                conf->cy = conf->rowoff + conf->screen_rows - 1;
                if (conf->cy > conf->numrows) conf->cy = conf->numrows;
            }
            while (times--) {
                editor_cursor_move(conf, c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
            }
            break;

        case HOME_KEY:
            conf->cx = 0;
            break;

        case END_KEY:
            if (row) {
                conf->cx = row->size;
            }
            break;

        case BACKSPACE:
        case CTRL_KEY('h'):
        case DEL_KEY:
            if (c == DEL_KEY) {
                editor_cursor_move(conf, ARROW_RIGHT);
            }

            if (time_elapsed > 0.5) {
                s = malloc(sizeof(struct Snapshot));
                if (!s) die("snapshot malloc failed");
                snapshot_create(conf, s);
                stack_push(conf->stack_undo, s);
            }
            editor_delete_char(conf);
            conf->last_time_modified = current_time;
            break;

        case CTRL_KEY('q'):
            if (quit_times != 0 && conf->flags.is_dirty) {
                editor_set_status_message(conf,
                                          "WARNING!!! file has unsaved changes."
                                          "Press CTRL-Q %d time(s) to leave",
                                          quit_times);
                quit_times--;
                return EXIT_FAILURE;
            }
            return EXIT_LOOP_CODE;
            break;
        case CTRL_KEY('l'):
        case '\x1b':
            break;

        case CTRL_KEY('s'):
            editor_save(conf);
            break;

        case CTRL_KEY('c'):
            editor_copy(conf);
            break;
        case CTRL_KEY('v'):
            s = malloc(sizeof(struct Snapshot));
            if (!s) die("snapshot malloc failed");
            snapshot_create(conf, s);
            stack_push(conf->stack_undo, s);
            conf->last_time_modified = current_time;

            editor_paste(conf);
            break;
        case CTRL_KEY('x'):
            s = malloc(sizeof(struct Snapshot));
            if (!s) die("snapshot malloc failed");
            snapshot_create(conf, s);
            stack_push(conf->stack_undo, s);
            conf->last_time_modified = current_time;

            editor_cut(conf);
            break;
        case CTRL_KEY('f'):
            editor_find(conf);
            break;
        case CTRL_KEY('z'):
            editor_undo(conf);
            break;
        case CTRL_KEY('y'):
            editor_redo(conf);
            break;
        default:
            // program officially starts
            if (!conf->flags.program_state) conf->flags.program_state = 1;

            if (time_elapsed > 0.5) {
                s = malloc(sizeof(struct Snapshot));
                if (!s) die("snapshot malloc failed");
                snapshot_create(conf, s);
                stack_push(conf->stack_undo, s);
            }

            editor_insert_char(conf, c);

            if (check_is_paranthesis(c)) {
                char extra_appended = closing_paren(c);
                if (c) {
                    editor_insert_char(conf, extra_appended);
                    if (!conf->cx) conf->cx--;
                }
            }

            conf->last_time_modified = current_time;

            break;
    }

    // we reset if user entered something else than a CTRL-Q
    quit_times = QUIT_TIMES;

    return EXIT_SUCCESS;
}

int8_t editor_insert_newline(struct EditorConfig *conf) {
    struct Row *current_row;
    if (conf->numrows) {
        current_row = &conf->rows[conf->cy];
    } else {
        editor_insert_row(conf, 0, "", 0);
        current_row = &conf->rows[0];
        return EXIT_SUCCESS;
    }

    int32_t numline_prefix_width = editor_row_numline_calculate(current_row);
    int32_t original_indent = current_row->indentation;
    int32_t new_indent = current_row->indentation;
    int8_t result;

    if (conf->cx == numline_prefix_width) {
        result = editor_insert_row(conf, conf->cy, "", 0);
    } else {
        int8_t is_compound_block =
            check_compound_statement(current_row->chars, current_row->size);
        int8_t cursor_inside_brackets =
            check_is_in_brackets(current_row->chars, current_row->size,
                                 conf->cx - numline_prefix_width);

        // cursor inside {} basically
        if (is_compound_block && cursor_inside_brackets) {
            char *bracket_remainder_start = strstr(current_row->chars, "}");
            int32_t bracket_pos = bracket_remainder_start - current_row->chars;
            int32_t remainder_length = current_row->size - bracket_pos;

            char *indented_remainder =
                malloc(remainder_length + new_indent + 1);
            int32_t indented_remainder_len = remainder_length + new_indent;

            memset(indented_remainder, '\t', new_indent);
            memcpy(indented_remainder + new_indent, bracket_remainder_start,
                   remainder_length);
            indented_remainder[indented_remainder_len] = '\0';

            current_row->chars = realloc(current_row->chars, bracket_pos + 1);
            current_row->chars[bracket_pos] = '\0';

            char *truncated_line = strdup(current_row->chars);
            int32_t truncated_line_len = strlen(truncated_line);

            editor_delete_row(conf, conf->cy);
            editor_insert_row(conf, conf->cy, truncated_line,
                              truncated_line_len);

            new_indent++;
            char *empty_indented_line = malloc(new_indent + 1);
            memset(empty_indented_line, '\t', new_indent);
            empty_indented_line[new_indent] = '\0';

            editor_insert_row(conf, conf->cy + 1, empty_indented_line,
                              new_indent);
            result = editor_insert_row(conf, conf->cy + 2, indented_remainder,
                                       indented_remainder_len);

            free(indented_remainder);
            free(truncated_line);
            free(empty_indented_line);
        } else {
            char *newline;
            int32_t newline_len;
            editor_row_indent(conf, current_row, &newline, &newline_len);
            new_indent = count_first_tabs(newline, newline_len);

            result =
                editor_insert_row(conf, conf->cy + 1, newline, newline_len);
            free(newline);

            current_row = &conf->rows[conf->cy];
            current_row->size = conf->cx - numline_prefix_width;

            current_row->chars =
                realloc(current_row->chars, current_row->size + 1);
            current_row->chars[current_row->size] = '\0';
            editor_update_row(conf, current_row);
        }
    }

    current_row = &conf->rows[conf->cy];

    int32_t updated_prefix_width = count_digits(current_row->idx + 2) + 1;
    numline_prefix_width =
        updated_prefix_width;  // could have been updated, so we check

    // moving cursor and rowoff
    if (result == EXIT_SUCCESS && numline_prefix_width) {
        conf->cx = numline_prefix_width + current_row->indentation;
        conf->cy++;
        if (conf->cy >= conf->rowoff + conf->screen_rows) {
            conf->rowoff++;
        }
        if (new_indent > original_indent) {
            conf->cx++;
        } else if (new_indent < original_indent) {
            if (!conf->cx) conf->cx--;
            if (conf->cx < numline_prefix_width) {
                conf->cx = numline_prefix_width;
            }
        }

        if (conf->cx < numline_prefix_width) {
            conf->cx = numline_prefix_width;
        }
    }

    return result;
}
