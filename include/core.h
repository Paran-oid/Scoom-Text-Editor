#ifndef CORE_H
#define CORE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

enum EditorKey;

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

/* Miscellaneous */

static inline void _die(const char* msg, const char* file, int32_t line,
                        const char* func) {
    fprintf(stderr, "[%s:%d in %s] \r\nFatal: %s\r\n", file, line, func, msg);
    exit(EXIT_FAILURE);
}
#define die(msg) (_die(msg, __FILE__, __LINE__, __func__))

char closing_paren(char c);

/* String and Char operations*/

#define CTRL_KEY(c) ((c) & 0x1f)
uint8_t str_append(char** dest, const char* src);
uint8_t str_prepend(char** dest, const char* src);

/* Checking */

#define ISCHAR(c) (('A' <= (c) && (c) <= 'Z') || ('a' <= (c) && (c) <= 'z'))
uint8_t check_seperator(char c);
uint8_t check_compound_statement(const char* str, size_t len);
uint8_t check_is_in_brackets(const char* str, size_t len, const uint32_t cx);
uint8_t check_is_paranthesis(enum EditorKey c);

/* Counting */

uint32_t count_char(const char* str, size_t size, enum EditorKey c);
uint32_t count_digits(const int32_t n);
uint32_t count_first_tabs(const char* s, size_t len);
uint32_t count_first_spaces(const char* s, size_t len);

/* Memory */

uint8_t swap(void* a, void* b, size_t elsize);

#endif