#ifndef CORE_H
#define CORE_H

typedef unsigned long size_t;

// MISC
void die(const char* s);

// MEMORY
int swap(void* a, void* b, size_t elsize);

// CHAR

#define ISCHAR(c) (('A' <= (c) && (c) <= 'Z') || ('a' <= (c) && (c) <= 'z'))

// STRING

#endif