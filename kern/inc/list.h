/**
 * @file list.h
 * @author Tony Xi (xiaolix)
 * @author Zekun Ma (zekunm)
 * @brief templates of the generic doubly linked list 
 */

#ifndef LIST_H_SEEN
#define LIST_H_SEEN

#include <stddef.h> // NULL
#include <malloc.h> // malloc
#include <assert.h> // affirm

#define DEFINE_NODE_T(node_t, data_t) \
    typedef struct node_t { \
        struct node_t *previous; \
        struct node_t *next; \
        data_t data; \
    } node_t
#define PUSH_FRONT(node_t, front, new_data, success) \
    { \
        node_t *new_front = malloc(sizeof(node_t)); \
        success = (new_front != NULL); \
        if (success) { \
            new_front->data = (new_data); \
            \
            if ((front) == NULL) { \
                new_front->previous = new_front; \
                new_front->next = new_front; \
            } else { \
                new_front->previous = (front)->previous; \
                new_front->next = (front); \
                new_front->previous->next = new_front; \
                new_front->next->previous = new_front; \
            } \
            \
            (front) = new_front; \
        } \
    }
#define PUSH_BACK(node_t, front, new_data, success) \
    { \
        node_t *new_back = malloc(sizeof(node_t)); \
        success = (new_back != NULL); \
        if (success) { \
            new_back->data = (new_data); \
            \
            if ((front) == NULL) { \
                new_back->previous = new_back; \
                new_back->next = new_back; \
                (front) = new_back; \
            } else { \
                new_back->previous = (front)->previous; \
                new_back->next = (front); \
                new_back->previous->next = new_back; \
                new_back->next->previous = new_back; \
            } \
        } \
    }
#define POP_FRONT(node_t, front) \
    { \
        affirm((front) != NULL); \
        node_t *old_front = (front); \
        if ((front) == (front)->next) { \
            (front) = NULL; \
        } else { \
            (front) = (front)->next; \
        } \
         \
        old_front->previous->next = old_front->next; \
        old_front->next->previous = old_front->previous; \
         \
        free(old_front); \
    }
#define POP_BACK(node_t, front) \
    { \
        affirm((front) != NULL); \
        node_t *old_back = (front)->previous; \
        if ((front) == (front)->previous) { \
            (front) = NULL; \
        } \
         \
        old_back->previous->next = old_back->next; \
        old_back->next->previous = old_back->previous; \
         \
        free(old_back); \
    }

#endif // LIST_H_SEEN
