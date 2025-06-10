#ifndef DLIST_H
#define DLIST_H

#include <stddef.h>
struct DListNode {
    void* data;

    struct DListNode* next;
    struct DListNode* prev;
};

struct DList {
    struct DListNode* head;
    struct DListNode* tail;
    size_t elsize;
    size_t size;

    int (*destroy)(void*);
    int (*cmp)(const void*, const void*);
};

#define dlist_size(dlist) ((dlist)->size)
#define dlist_head(dlist) ((dlist)->head)
#define dlist_tail(dlist) ((dlist)->tail)

#define dlist_is_head(tist, elem) ((dlist)->head == (elem) ? 1 : 0)
#define dlist_is_tail(dlist, elem) ((dlist)->tail == (elem) ? 1 : 0)

#define dlist_data(elem) ((elem)->data)
#define dlist_next(elem) ((elem)->next)
#define dlist_prev(elem) ((elem)->prev)

int node_create(struct DListNode* node, struct DListNode* next,
                struct DListNode* prev, void* data, size_t elsize);
int dlist_create(struct DList* list, size_t elsize, int (*destroy)(void*),
                 int (*cmp)(const void*, const void*));
int dlist_insert_after(struct DList* list, struct DListNode* prev_node,
                       const void* data);
int dlist_remove(struct DList* list, struct DListNode* node, void** data);
int dlist_free(struct DList* list);

#endif