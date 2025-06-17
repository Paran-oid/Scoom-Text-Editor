#include "input.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "config.h"
#include "core.h"
#include "file.h"
#include "render.h"
#include "rows.h"

int editor_cursor_ctrl(struct EditorConfig *conf, enum EditorKey key) {
    if (conf->cy < 0 || conf->cy >= conf->numrows) return EXIT_FAILURE;

    struct Row *row = &conf->rows[conf->cy];
    int numline_size = editor_row_numline_calculate(row);
    if (key == CTRL_ARROW_RIGHT) {
        if (conf->cx == (int)row->size + numline_size) {
            conf->cy++;
            if (conf->cy >= conf->numrows - 1) {
                conf->cy--;
                return EXIT_FAILURE;
            }
            conf->cx = numline_size;
            row = &conf->rows[conf->cy];
        }

        // try to find the first item that is not a char or _
        while (conf->cx < (int)row->size + numline_size &&
               !ISCHAR(row->chars[conf->cx - numline_size]) &&
               (row->chars[conf->cx - numline_size] != '_'))
            conf->cx++;

        while (conf->cx < (int)row->size + numline_size &&
               (ISCHAR(row->chars[conf->cx - numline_size]) ||
                (row->chars[conf->cx - numline_size] == '_')))
            conf->cx++;

    } else {
        while (conf->cx != numline_size &&
               (row->chars[conf->cx - numline_size] == '\t' ||
                row->chars[conf->cx - numline_size - 1] == '\t'))
            conf->cx--;

        if (conf->cx == numline_size) {
            conf->cy--;
            if (conf->cy < 0) {
                conf->cy = 0;
                return EXIT_FAILURE;
            }
            row = &conf->rows[conf->cy];
            conf->cx = row->size != 0 ? (int)row->size - 1 : numline_size;
        }

        if (conf->cx != numline_size) {
            conf->cx--;
            while (conf->cx > numline_size &&
                   !ISCHAR(row->chars[conf->cx - numline_size]) &&
                   (row->chars[conf->cx - numline_size] != '_'))
                conf->cx--;

            while (conf->cx > numline_size &&
                   (ISCHAR(row->chars[conf->cx - numline_size]) ||
                    (row->chars[conf->cx - numline_size] == '_')))
                conf->cx--;

            if (!ISCHAR(row->chars[conf->cx - numline_size]) &&
                (row->chars[conf->cx - numline_size] != '_')) {
                conf->cx++;
            }
        }
    }
    return EXIT_SUCCESS;
}

int editor_cursor_move(struct EditorConfig *conf, int key) {
    struct Row *row =
        (conf->cy >= conf->numrows) ? NULL : &conf->rows[conf->cy];

    int numline_offset_size = 0;
    if (row) {
        numline_offset_size = editor_row_numline_calculate(row);
    }

    int desired_cx_logical = conf->cx - numline_offset_size;

    switch (key) {
        case ARROW_LEFT:
            if (conf->cx != numline_offset_size) {
                if (conf->coloff != 0 &&
                    conf->cx == numline_offset_size + conf->coloff) {
                    conf->coloff--;
                }
                conf->cx--;
            } else if (conf->cy > 0) {
                conf->cy--;
                row =
                    (conf->cy >= conf->numrows) ? NULL : &conf->rows[conf->cy];
                numline_offset_size = editor_row_numline_calculate(row);
                conf->cx = row->size + numline_offset_size;
            }
            break;
        case ARROW_RIGHT:
            if (row && conf->cx < (int)row->size + numline_offset_size) {
                conf->cx++;
            } else if (row && conf->cy < conf->numrows - 1) {
                row = &conf->rows[++conf->cy];
                numline_offset_size =
                    editor_row_numline_calculate(row);  // recalculate it
                conf->cx = numline_offset_size;
            }
            break;
        case ARROW_UP:
            if (conf->cy > 0) {
                conf->cy--;
                row = &conf->rows[conf->cy];
                numline_offset_size = editor_row_numline_calculate(row);
                if ((size_t)desired_cx_logical > row->size)
                    desired_cx_logical = row->size;
                conf->cx = numline_offset_size + desired_cx_logical;
            }
            break;
        case ARROW_DOWN:
            if (conf->cy < conf->numrows - 1) {
                conf->cy++;
                row = &conf->rows[conf->cy];
                numline_offset_size = editor_row_numline_calculate(row);
                if ((size_t)desired_cx_logical > row->size)
                    desired_cx_logical = row->size;
                conf->cx = numline_offset_size + desired_cx_logical;
            }
            break;
        default:
            return EXIT_FAILURE;
    }
    row = (conf->cy >= conf->numrows) ? NULL : &conf->rows[conf->cy];
    if (row && conf->cx > (int)row->size + numline_offset_size) {
        conf->cx = row->size;
    }

    return EXIT_SUCCESS;
}

int editor_read_key(void) {
    char c;
    int nread;
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if (nread == -1 && errno != EAGAIN) die("read");
    }

    if (c == '\x1b') {
        char seq[5];
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

    return EXIT_SUCCESS;
}

/*
        Side Note:
        We use int c instead of char c since we have mapped
        some keys to values bigger than the max 255 like for ARROWS
*/
int editor_process_key_press(struct EditorConfig *conf) {
    static int quit_times = QUIT_TIMES;

    struct Snapshot *s;
    struct Row *row =
        (conf->cy >= conf->numrows) ? NULL : &conf->rows[conf->cy];
    int c = editor_read_key();

    int times = conf->screen_rows;  // this will be needed in case of page
                                    // up or down basically

    time_t current_time = time(NULL);
    double time_elapsed = difftime(current_time, conf->last_time_modified);

    switch (c) {
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
                // cursor jumps up to previous page
                conf->cy = conf->rowoff;
            } else if (c == PAGE_DOWN) {
                // cursor jumps up to next page
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
                if (!s) return EXIT_FAILURE;
                snapshot_create(conf, s);
                stack_push(conf->stack_undo, s);
            }

            editor_delete_char(conf);
            conf->last_time_modified = current_time;
            break;

        case CTRL_KEY('q'):
            if (quit_times != 0 && conf->is_dirty) {
                editor_set_status_message(conf,
                                          "WARNING!!! file has unsaved changes."
                                          "Press CTRL-Q %d time(s) to leave",
                                          quit_times);
                quit_times--;
                return EXIT_SUCCESS;
            }
            return EXIT_CODE;
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
            if (!s) return EXIT_FAILURE;
            snapshot_create(conf, s);
            stack_push(conf->stack_undo, s);
            conf->last_time_modified = current_time;

            editor_paste(conf);
            break;
        case CTRL_KEY('x'):
            s = malloc(sizeof(struct Snapshot));
            if (!s) return EXIT_FAILURE;
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
            if (time_elapsed > 0.5) {
                s = malloc(sizeof(struct Snapshot));
                if (!s) return EXIT_FAILURE;
                snapshot_create(conf, s);
                stack_push(conf->stack_undo, s);
            }

            editor_insert_char(conf, c);
            conf->last_time_modified = current_time;

            break;
    }

    // we reset if user entered something else than a CTRL-Q
    quit_times = QUIT_TIMES;

    return EXIT_SUCCESS;
}

int editor_insert_newline(struct EditorConfig *conf) {
    struct Row *row;
    if (conf->numrows) {
        row = &conf->rows[conf->cy];
    } else {
        editor_insert_row(conf, 0, "", 0);
        row = &conf->rows[0];
        return EXIT_SUCCESS;
    }
    int numline_offset_size = editor_row_numline_calculate(row);

    // TODO: make sure when you start from an empty file there won't be any bug
    int res;
    if (conf->cx == numline_offset_size) {
        res = editor_insert_row(conf, conf->cy, "", 0);
    } else {
        res = editor_insert_row(conf, conf->cy + 1,
                                &row->chars[conf->cx - numline_offset_size],
                                row->size - conf->cx + numline_offset_size);

        row = &conf->rows[conf->cy];  // realloc could make our pointer invalid
                                      // so better safe than sorry

        row->size = conf->cx - numline_offset_size;
        row->chars[row->size] = '\0';
        editor_update_row(conf, row);

        // numline could have changed to a bigger degree
        // so an update is necessary without the already made up call
        int new_temp = count_digits(row->idx + 2) + 1;
        if (new_temp != numline_offset_size) {
            numline_offset_size = new_temp;
        }
    }
    if (res == 0 && numline_offset_size) {
        conf->cx = numline_offset_size, conf->cy++;
    }
    return res;
}