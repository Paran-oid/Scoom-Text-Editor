#include "core.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "stack.h"

/* Misc */

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
int8_t str_append(char** dest, const char* src) {
    if (!dest || !src) die("tried to append to an empty string");

    int32_t dest_len = strlen(*dest);
    int32_t src_len = strlen(src);

    char* result = malloc(src_len + dest_len + 1);
    if (!result) die("error allocating memory for str_append");

    memcpy(result, *dest, dest_len);
    memcpy(result + dest_len, src, src_len);
    result[dest_len + src_len] = '\0';

    free(*dest);
    *dest = result;

    return EXIT_SUCCESS;
}

int8_t str_prepend(char** dest, const char* src) {
    if (!dest || !src) die("tried to prepend to an empty string");

    int32_t dest_len = strlen(*dest);
    int32_t src_len = strlen(src);

    char* result = malloc(src_len + dest_len + 1);
    if (!result) die("error allocating memory for str_prepend");

    memcpy(result, src, src_len);
    memcpy(result + src_len, *dest, dest_len);
    result[dest_len + src_len] = '\0';

    free(*dest);
    *dest = result;

    return EXIT_SUCCESS;
}

/* Checking */

int8_t check_seperator(char c) {
    return isspace(c) || c == '\0' || strchr(",.()+-/*=~%<>[];", c) != NULL;
}

int8_t check_compound_statement(const char* str, int32_t len) {
    if (count_char(str, len, '{') == 0 && count_char(str, len, '}') == 0)
        return 0;

    Stack* s = malloc(sizeof(Stack));
    if (!s) die("stack 's' malloc failed...");

    stack_create(s, NULL, free);

    int8_t in_string = 0;

    for (int32_t i = 0; i < len; i++) {
        char c = str[i];

        if (c == '"' && (i == 0 || str[i - 1] != '\\')) {
            in_string = !in_string;
            continue;
        }
        if (!in_string) {
            char* data;
            if (c == '{') {
                data = strdup("{");
                if (!data) die("data strdup failed");

                stack_push(s, data);
            } else if (c == '}') {
                char* peaked = stack_peek(s);
                void* ptr;
                if (peaked && strcmp(peaked, "{") == 0) {
                    stack_pop(s, &ptr);
                    free(ptr);
                } else {
                    data = strdup("}");
                    if (!data) die("data strdup failed");

                    stack_push(s, data);
                }
            }
        }
    }

    int8_t isempty = stack_size(s) == 0;
    stack_destroy(s);
    free(s);

    return isempty;
}

int8_t check_is_in_brackets(const char* str, int32_t len, int32_t cx) {
    int32_t brackets = 0;
    int8_t in_string = 0;

    int32_t i = 0;
    for (; i < cx && i < len; i++) {
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

    if (brackets <= 0) return 0;

    while (i < len) {
        if (str[i] == '}') return 1;
    }

    return 0;
}

inline int8_t check_is_paranthesis(char c) {
    return c == '{' || c == '(' || c == '[';
}

/* Counting */

int32_t count_char(const char* str, const int32_t size, char c) {
    int32_t total = 0;
    for (int32_t i = 0; i < size; i++) {
        if (str[i] == c) total++;
    }
    return total;
}

int32_t count_digits(int32_t n) {
    if (n == 0) return 0;

    int32_t res = 0;
    while (n) {
        n /= 10;
        res++;
    }
    return res;
}

// this function basically calculates "indentations"
int32_t count_first_tabs(const char* s, int32_t len) {
    int8_t count = 0;
    for (int32_t i = 0; i < len; i++) {
        if (s[i] == '\t')
            count++;
        else
            break;
    }
    return count;
}

// same functionality as above just with spaces
int32_t count_first_spaces(const char* s, int32_t len) {
    int32_t count = 0;
    for (int32_t i = 0; i < len; i++) {
        if (s[i] == ' ')
            count++;
        else
            break;
    }
    return count;
}

// memory

int8_t swap(void* a, void* b, int32_t elsize) {
    void* temp = malloc(elsize);
    if (!temp) die("memory allocation for temp in swap failed");

    memcpy(temp, a, elsize);
    memcpy(a, b, elsize);
    memcpy(b, temp, elsize);

    free(temp);

    return EXIT_SUCCESS;
}
