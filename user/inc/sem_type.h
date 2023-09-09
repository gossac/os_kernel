/** @file sem_type.h
 *  @brief This file defines the type for semaphores.
 */

#ifndef _SEM_TYPE_H
#define _SEM_TYPE_H

#include <mutex.h>
#include <cond.h>

typedef struct sem {

  // Semaphore count
  int count;

  // Mutex to lock the struct and modify internal states
  mutex_t lock;

  // Cvar to wait on when no count is left
  cond_t cond;
} sem_t;

#endif /* _SEM_TYPE_H */
