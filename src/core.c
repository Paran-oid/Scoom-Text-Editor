#include "core.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "config.h"

/* Misc */

void die(const char* s) {
    perror(s);
    exit(EXIT_FAILURE);
}

// acceptable parms: {, (, [
char closing_paren(char c) {
    switch (c) {
        case '{':
            return '}';
        case '(':
            return ')';
        case '[':
            return ']';
        default:
            return '\0';
    }
}

/* String and Char operations*/
int str_append(char** dest, const char* src) {
    if (!dest || !src) {
        return NULL_PARAMETER;
    }

    size_t dest_len = strlen(*dest);
    size_t src_len = strlen(src);

    char* result = malloc(src_len + dest_len + 1);
    if (!result) return OUT_OF_MEMORY;

    memcpy(result, *dest, dest_len);
    memcpy(result + dest_len, src, src_len);
    result[dest_len + src_len] = '\0';

    free(*dest);
    *dest = result;

    return SUCCESS;
}

int str_prepend(char** dest, const char* src) {
    if (!dest || !src) {
        return NULL_PARAMETER;
    }

    size_t dest_len = strlen(*dest);
    size_t src_len = strlen(src);

    char* result = malloc(src_len + dest_len + 1);
    if (!result) return OUT_OF_MEMORY;

    memcpy(result, src, src_len);
    memcpy(result + src_len, *dest, dest_len);
    result[dest_len + src_len] = '\0';

    free(*dest);
    *dest = result;

    return SUCCESS;
}

/* Checking */

bool check_seperator(unsigned char c) {
    return isspace(c) || c == '\0' || strchr(",.()+-/*=~%<>[];", c) != NULL;
}

bool check_compound_statement(const char* str, size_t len) {
    if (count_char(str, len, '{') == 0 && count_char(str, len, '}') == 0) {
        return false;
    }

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

    bool isempty = stack_size(s) == 0;
    stack_destroy(s);
    free(s);

    return isempty;
}

bool check_is_in_brackets(const char* str, size_t len, int cx) {
    int brackets = 0;
    bool in_string = false;

    size_t i = 0;
    for (; i < (size_t)cx && i < len; i++) {
        char c = str[i];

        if (c == '"' && (i == 0 || str[i - 1] != '\\')) {
            in_string = !in_string;
            continue;
        }

        if (!in_string) {
            if (c == '{') {
                brackets++;
            } else if (c == '}') {
                if (brackets > 0) brackets--;
            } else {
                brackets = 0;
            }
        }
    }

    if (brackets <= 0) return false;

    while (i < len) {
        if (str[i] == '}') return true;
    }

    return false;
}

inline bool check_is_paranthesis(char c) {
    return c == '{' || c == '(' || c == '[';
}

/* Counting */

int count_char(const char* str, const size_t size, char c) {
    int total = 0;
    for (size_t i = 0; i < size; i++) {
        if (str[i] == c) total++;
    }
    return total;
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
int count_first_tabs(const char* s, size_t len) {
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
int count_first_spaces(const char* s, size_t len) {
    int count = 0;
    for (size_t i = 0; i < len; i++) {
        if (s[i] == ' ')
            count++;
        else
            break;
    }
    return count;
}

// memory

int swap(void* a, void* b, size_t elsize) {
    void* temp = malloc(elsize);
    if (!temp) return OUT_OF_MEMORY;

    memcpy(temp, a, elsize);
    memcpy(a, b, elsize);
    memcpy(b, temp, elsize);

    free(temp);

    return SUCCESS;
}
