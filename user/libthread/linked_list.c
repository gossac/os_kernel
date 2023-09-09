/**
 * @file linked_list.c
 * @brief Operations on circular doubly linked lists
 * 
 * @author Tony Xi (xiaolix)
 * @author Zekun Ma (zekunm)
 * @bug no known bug
 */

#include <linked_list.h> // list_node
#include <malloc.h> // malloc
#include <stddef.h> // NULL

list_node *append_node(list_node *head, void *data) {
    list_node *new_node_ptr = malloc(sizeof(list_node));
    if (new_node_ptr == NULL) {
        return head;
    }
    new_node_ptr->data = data;
    if (head == NULL) {
        new_node_ptr->next = new_node_ptr;
        new_node_ptr->previous = new_node_ptr;
        head = new_node_ptr;
    } else {
        new_node_ptr->next = head;
        new_node_ptr->previous = head->previous;
        new_node_ptr->next->previous = new_node_ptr;
        new_node_ptr->previous->next = new_node_ptr;
    }
    return head;
}

list_node *remove_node(list_node *head, list_node *node_ptr) {
    if (head == NULL) {
        return NULL;
    }
    list_node *current_node_ptr = head;
    do {
        if (current_node_ptr == node_ptr) {
            if (head == current_node_ptr) {
                if (head == head->next) {
                    head = NULL;
                } else {
                    head = head->next;
                }
            }
            current_node_ptr->previous->next = current_node_ptr->next;
            current_node_ptr->next->previous = current_node_ptr->previous;
            free(current_node_ptr);
            break;
        }
        current_node_ptr = current_node_ptr->next;
    } while (current_node_ptr != head);
    return head;
}