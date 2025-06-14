#ifndef CORE_H
#define CORE_H

#include <stdbool.h>
#include <stddef.h>

/* Miscellaneous */

#define CTRL_KEY(c) ((c) & 0x1f)
#define ISCHAR(c) (('A' <= (c) && (c) <= 'Z') || ('a' <= (c) && (c) <= 'z'))

void die(const char* s);
bool is_separator(unsigned char c);

int count_digits(int n);

/* Memory */
int swap(void* a, void* b, size_t elsize);

#endif