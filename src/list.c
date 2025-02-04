#include "ping.h"

inline struct Node *list_prepend(struct Node *head, double x) {
    struct Node *node = (struct Node *)malloc(sizeof(*head));

    if (!node)
        return NULL;

    node->x = x;

    node->next = head;

    return node;
}

inline void list_free(struct Node *head) {
    for (struct Node *iter = head; iter;) {
        struct Node *tmp = iter;

        iter = iter->next;

        free(tmp);
    }
}
