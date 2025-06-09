#include "dlist.h"

#include <stdlib.h>
#include <string.h>

int node_create(struct DListNode* node, struct DListNode* next,
                struct DListNode* prev, void* data, size_t elsize) {
    if (!node) return EXIT_FAILURE;

    node->prev = prev;
    node->next = next;

    if (data) {
        node->data = malloc(elsize);
        memcpy(node->data, data, elsize);
    }

    return EXIT_SUCCESS;
}

int list_create(struct DList* list, size_t elsize, int (*destroy)(void*),
                int (*cmp)(const void*, const void*)) {
    if (!list) return EXIT_FAILURE;

    list->head = list->tail = NULL;
    list->size = 0;
    list->elsize = elsize;

    list->cmp = cmp;
    list->destroy = destroy;

    return EXIT_SUCCESS;
}

int list_insert_after(struct DList* list, struct DListNode* prev_node,
                      const void* data) {
    if (!list || !data) return EXIT_FAILURE;

    struct DListNode* node = malloc(sizeof(struct DListNode));
    if (!node) return EXIT_FAILURE;

    if (!prev_node) {
        if (node_create(node, NULL, NULL, (void*)data, list->elsize) ==
            EXIT_FAILURE) {
            free(node);
            return EXIT_FAILURE;
        }

        if (!list->head) {
            list->head = node;
            list->tail = node;
        } else {
            node->next = list->head;
            list->head->prev = node;
            list->head = node;
        }
    } else {
        if (node_create(node, prev_node->next, prev_node, (void*)data,
                        list->elsize) == EXIT_FAILURE) {
            free(node);
            return EXIT_FAILURE;
        }

        if (prev_node == list->tail) {
            list->tail = node;
        }

        if (prev_node->next) prev_node->next->prev = node;
        prev_node->next = node;
    }

    list->size++;
    return EXIT_SUCCESS;
}

int list_remove(struct DList* list, struct DListNode* node, void** data) {
    if (!list || !list->head) return EXIT_FAILURE;

    struct DListNode* to_remove;
    if (!node) {
        to_remove = list->head;
    } else {
        to_remove = node;
    }

    if (data) {
        memcpy(*data, to_remove->data, list->elsize);
    }

    if (to_remove == list->head) {
        list->head = to_remove->next;
    }

    if (to_remove == list->tail) {
        list->tail = to_remove->prev;
    }

    if (to_remove->prev) {
        to_remove->prev->next = to_remove->next;
    }

    if (to_remove->next) {
        to_remove->next->prev = to_remove->prev;
    }

    list->destroy(to_remove);
    list->size--;
    return EXIT_SUCCESS;
}

int list_free(struct DList* list) {
    if (!list) return EXIT_FAILURE;

    if (list->destroy) {
        while (list->size) {
            list_remove(list, NULL, NULL);
        }
    }

    return EXIT_SUCCESS;
}