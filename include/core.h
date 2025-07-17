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
int8_t str_append(char** dest, const char* src);
int8_t str_prepend(char** dest, const char* src);

/* Checking */

#define ISCHAR(c) (('A' <= (c) && (c) <= 'Z') || ('a' <= (c) && (c) <= 'z'))
int8_t check_seperator(char c);
int8_t check_compound_statement(const char* str, int32_t len);
int8_t check_is_in_brackets(const char* str, int32_t len, const int32_t cx);
int8_t check_is_paranthesis(char c);

/* Counting */

int32_t count_char(const char* str, int32_t size, char c);
int32_t count_digits(const int32_t n);
int32_t count_first_tabs(const char* s, int32_t len);
int32_t count_first_spaces(const char* s, int32_t len);

/* Memory */

int8_t swap(void* a, void* b, int32_t elsize);

#endif