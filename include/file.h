#ifndef FILE_H
#define FILE_H

#define DEBUG

#include <stddef.h>

struct EditorConfig;
struct ABuf;

struct Snapshot {
    char* text;
    size_t len;

    int numrows;
    int cx, cy;
};

int snapshot_create(struct EditorConfig* conf, struct Snapshot* snapshot);
int snapshot_destroy(struct Snapshot* snapshot);

int editor_open(struct EditorConfig* conf, const char* path);
int editor_run(struct EditorConfig* conf);
int editor_destroy(struct EditorConfig* conf);
int editor_save(struct EditorConfig* conf);

int editor_undo(struct EditorConfig* conf);
int editor_redo(struct EditorConfig* conf);

int editor_copy(struct EditorConfig* conf);
int editor_paste(struct EditorConfig* conf);
int editor_cut(struct EditorConfig* conf);

int editor_find(struct EditorConfig* conf);
int editor_go_to(struct EditorConfig* conf);
int editor_extract_filename(struct EditorConfig* conf, char** filename);

#endif