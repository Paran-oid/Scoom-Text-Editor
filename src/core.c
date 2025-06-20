#include "core.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

// MISC

void die(const char* s) {
    perror(s);
    exit(EXIT_FAILURE);
}

bool check_seperator(unsigned char c) {
    return isspace(c) || c == '\0' || strchr(",.()+-/*=~%<>[];", c) != NULL;
}

bool check_compound_statement(char* str, size_t len) {
    Stack* s = malloc(sizeof(Stack));
    stack_create(s, NULL, free);

    bool in_string = false;

    for (size_t i = 0; i < len; i++) {
        char c = str[i];

        if (c == '"' && (i == 0 || str[i - 1] != '\\')) {
            in_string = !in_string;
            continue;
        }
        if (!in_string) {
            char* data;
            if (c == '{') {
                data = strdup("{");
                stack_push(s, data);
            } else if (c == '}') {
                char* peaked = stack_peek(s);
                void* ptr;
                if (peaked && strcmp(peaked, "{") == 0) {
                    stack_pop(s, &ptr);
                    free(ptr);
                } else {
                    data = strdup("}");
                    stack_push(s, data);
                }
            }
        }
    }

    /*
        If we encounter an open bracket we increase
        identation, else we decrease it
    */

    bool isempty = stack_size(s) == 0;
    free(s);

    return isempty;
}

int count_digits(int n) {
    if (n == 0) return INVALID_ARG;

    int res = 0;
    while (n) {
        n /= 10;
        res++;
    }
    return res;
}

// this function basically calculates "indentations"
int count_first_tabs(char* s, size_t len) {
    int count = 0;
    for (size_t i = 0; i < len; i++) {
        if (s[i] == '\t')
            count++;
        else
            break;
    }
    return count;
}

// same functionality as above just with spaces
int count_first_spaces(char* s, size_t len) {
    int count = 0;
    for (size_t i = 0; i < len; i++) {
        if (s[i] == ' ')
            count++;
        else
            break;
    }
    return count;
}

// MEMORY

int swap(void* a, void* b, size_t elsize) {
    void* temp = malloc(elsize);
    if (!temp) return OUT_OF_MEMORY;

    memcpy(temp, a, elsize);
    memcpy(a, b, elsize);
    memcpy(b, temp, elsize);

    free(temp);

    return SUCCESS;
}
