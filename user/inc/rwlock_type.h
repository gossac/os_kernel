/**
 * @file rwlock_type.h
 * @brief This file defines the type for reader/writer locks.
 * 
 * @author Tony Xi (xiaolix), Zekun Ma (zekunm)
 * @bug No known bug.
 */

#ifndef _RWLOCK_TYPE_H
#define _RWLOCK_TYPE_H

#include <linked_list.h> // list_node
#include <mutex.h> // mutex_t
#include <cond.h> // cond_t

typedef struct rwlock {
    // a list of reader and writer threads,
    // either holding the lock now or waiting the lock,
    // in the order of lock assignment
    list_node *rw_list;

    // protect the rw_list
    mutex_t rw_list_manipulation;
} rwlock_t;

/**
 * @brief A rw_record, as the data field of a rw_list node,
 *        represents a reader or writer thread in the queue.
 */
typedef struct rw_record {
    // the ID of the reader/writer thread
    int thread_id;

    // the type of the lock the thread is holding
    // or is requesting, values defined in rwlock.h
    int rw_lock_type;

    // The thread should wait on this condition variable
    // if it is not allowed to get the lock for now.
    cond_t rw_lock_permission;
} rw_record;

#endif /* _RWLOCK_TYPE_H */
