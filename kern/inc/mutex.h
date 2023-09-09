/**
 * @file mutex.h
 *
 * @brief Defines functions for mutex.
 *
 * @author Tony Xi (xiaolix)
 * @author Zekun Ma (zekunm)
 * @bug No known bugs.
 **/

#ifndef _MUTEX_H
#define _MUTEX_H

typedef struct mutex {
    int lock_state;
    int tid;
} mutex_t;

int mutex_init(mutex_t *mp);
void mutex_destroy(mutex_t *mp);
void mutex_lock(mutex_t *mp);
int mutex_try_lock(mutex_t *mp);
void mutex_unlock(mutex_t *mp);

#endif /* _MUTEX_H */
