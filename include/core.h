#ifndef CORE_H
#define CORE_H

#include <stdbool.h>
#include <stddef.h>

/* Miscellaneous */

void process_init(void* (*func)(void*));
void die(const char* s);
char closing_paren(char c);

/* String and Char operations*/

#define CTRL_KEY(c) ((c) & 0x1f)
int str_append(char** dest, const char* src);
int str_prepend(char** dest, const char* src);

/* Checking */

#define ISCHAR(c) (('A' <= (c) && (c) <= 'Z') || ('a' <= (c) && (c) <= 'z'))
bool check_seperator(const unsigned char c);
bool check_compound_statement(const char* str, size_t len);
bool check_is_in_brackets(const char* str, size_t len, const int cx);
bool check_is_paranthesis(char c);

/* Counting */

int count_char(const char* str, size_t size, const char c);
int count_digits(const int n);
int count_first_tabs(const char* s, size_t len);
int count_first_spaces(const char* s, size_t len);

/* Memory */

int swap(void* a, void* b, const size_t elsize);

#endif