#ifndef __LINKED_LIST_H__
#define __LINKED_LIST_H__

/**
 * @brief the data structure of a generic circular doubly linked list
 */
typedef struct list_node {
    void *data;
    struct list_node *next;
    struct list_node *previous;
} list_node;

/**
 * @brief attach a node to the end of the list
 * 
 * @param head the head node of the list
 *        @code{NULL} if the list is empty
 * @param data data to be wrapped in the new node
 * @return the head node of the list after updated
 */
list_node *append_node(list_node *head, void *data);
/**
 * @brief remove a node from the list if
 *        the node does exists in the list
 * 
 * @param head the head node of the list,
 *        @code{NULL} if the list is empty
 * @param node_ptr the node to be removed
 * @return the head node of the list after updated
 */
list_node *remove_node(list_node *head, list_node *node_ptr);

#endif /* __LINKED_LIST_H__ */