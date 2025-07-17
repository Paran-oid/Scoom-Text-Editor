#ifndef FILE_H
#define FILE_H

#include <stddef.h>
#include <stdint.h>

struct EditorConfig;
struct ABuf;

struct Snapshot {
    char* text;

    int32_t len;
    int32_t numrows;

    int32_t cx, cy;
};

int8_t snapshot_create(struct EditorConfig* conf, struct Snapshot* snapshot);
int8_t snapshot_destroy(struct Snapshot* snapshot);

int8_t editor_open(struct EditorConfig* conf, const char* path);
int8_t editor_run(struct EditorConfig* conf);
int8_t editor_destroy(struct EditorConfig* conf);
int8_t editor_save(struct EditorConfig* conf);

int8_t editor_undo(struct EditorConfig* conf);
int8_t editor_redo(struct EditorConfig* conf);

int8_t editor_copy(struct EditorConfig* conf);
int8_t editor_paste(struct EditorConfig* conf);
int8_t editor_cut(struct EditorConfig* conf);

int8_t editor_find(struct EditorConfig* conf);
int8_t editor_extract_filename(struct EditorConfig* conf, char** filename);

#endif