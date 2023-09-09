/** @file cond_type.h
 *  @brief This file defines the type for condition variables.
 */

#ifndef _COND_TYPE_H
#define _COND_TYPE_H

#include <linked_list.h>

typedef struct cond {

  // Mutex to lock the struct and modify internal states
  mutex_t lock;

  // Head of the list storing waiting threads
  list_node *tid_head;

} cond_t;

#endif /* _COND_TYPE_H */
