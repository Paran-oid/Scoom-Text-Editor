#ifndef FILE_H
#define FILE_H

#include <stddef.h>
#include <stdint.h>

struct EditorConfig;
struct ABuf;

struct Snapshot {
    char* text;

    size_t len;
    size_t numrows;

    int32_t cx, cy;
};

uint8_t snapshot_create(struct EditorConfig* conf, struct Snapshot* snapshot);
uint8_t snapshot_destroy(struct Snapshot* snapshot);

uint8_t editor_open(struct EditorConfig* conf, const char* path);
uint8_t editor_run(struct EditorConfig* conf);
uint8_t editor_destroy(struct EditorConfig* conf);
uint8_t editor_save(struct EditorConfig* conf);

uint8_t editor_undo(struct EditorConfig* conf);
uint8_t editor_redo(struct EditorConfig* conf);

uint8_t editor_copy(struct EditorConfig* conf);
uint8_t editor_paste(struct EditorConfig* conf);
uint8_t editor_cut(struct EditorConfig* conf);

uint8_t editor_find(struct EditorConfig* conf);
uint8_t editor_extract_filename(struct EditorConfig* conf, char** filename);

#endif