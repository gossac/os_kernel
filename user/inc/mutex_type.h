/** @file mutex_type.h
 *  @brief This file defines the type for mutexes.
 */

#ifndef _MUTEX_TYPE_H
#define _MUTEX_TYPE_H

#define UNLOCKED    (1)
#define LOCKED      (0)

typedef struct mutex {
    int lock_state;
    int tid;
} mutex_t;

#endif /* _MUTEX_TYPE_H */
