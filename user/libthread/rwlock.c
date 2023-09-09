/**
 * @file rwlock.c
 * @brief implementation of reader/writer locks
 * 
 * @author Tony Xi (xiaolix)
 * @author Zekun Ma (zekunm)
 * @bug No known bug.
 */

#include <rwlock.h> // rwlock_init
#include <stddef.h> // NULL
#include <stdbool.h> // bool
#include <malloc.h> // malloc
#include <thread.h> // thr_getid

// helper functions for internal use
/**
 * @brief test if the reader/writer lock type is a valid one
 * 
 * @param type specify the type of a reader/writer lock
 * @return whether the reader/writer lock type is valid
 */
bool is_rw_lock_type(int type);
/**
 * @brief find the first thread from a reader/writer list
 * that requests or holds a specific type of lock
 * 
 * @param rw_list the reader/writer list
 * @param rw_lock_type the reader/writer lock type
 * @return the reader/writer list node of the first requestor
 *         or holder of such type of lock if any, @code{NULL}} otherwise
 */
list_node *find_by_rw_lock_type(list_node *rw_list, int rw_lock_type);
/**
 * @brief find the first thread from a
 * reader/writer list with the specific thread ID
 * 
 * @param rw_list the reader/writer list
 * @param thread_id the ID of the thread
 * @return list_node* the reader/writer list node of the thread with
 *         specific thread ID if any, @code{NULL} otherwise
 */
list_node *find_by_thread_id(list_node *rw_list, int thread_id);


/**
 * @brief Initialize the lock pointed to by rwlock. 
 * 
 * It is illegal for an application to use a readers/writers 
 * lock before it has been 
 * initialized or to initialize one when it is already 
 * initialized and in use.
 *
 * @param rwlock A pointer to a rwlock struct
 * @return 0 on success, -1 otherwise
 */
int rwlock_init(rwlock_t *rwlock) {
    // do argument checking
    if (rwlock == NULL) {
        return -1;
    }

    // Initialize each field.
    // Return -1 once any failure happens.
    rwlock->rw_list = NULL;
    if (mutex_init(&(rwlock->rw_list_manipulation)) != 0) {
        return -1;
    }

    // finally succeed
    return 0;
}

/**
 * @brief Blocks the calling thread until it has been granted the 
 * requested form of access
 * 
 * The type parameter is required to be either RWLOCK READ 
 * (for a shared lock) 
 * or RWLOCK WRITE (for an exclusive lock).
 *
 * @param rwlock A pointer to a rwlock struct
 * @param type An integer indicating the lock type
 */
void rwlock_lock(rwlock_t *rwlock, int type) {
    // do argument checking
    if (rwlock == NULL || !is_rw_lock_type(type)) {
        return;
    }

    // create a new record for this reader/writer thread
    rw_record *new_record_ptr = malloc(sizeof(rw_record));
    if (new_record_ptr == NULL) {
        return;
    }
    new_record_ptr->thread_id = thr_getid();
    new_record_ptr->rw_lock_type = type;
    if (cond_init(&(new_record_ptr->rw_lock_permission)) != 0) {
        free(new_record_ptr);
        return;
    }

    // insert the new record into rw_list and wait if necessary
    mutex_lock(&(rwlock->rw_list_manipulation));
    switch (new_record_ptr->rw_lock_type) {
        case RWLOCK_READ: {
            list_node *first_w_node_ptr = find_by_rw_lock_type(
                rwlock->rw_list,
                RWLOCK_WRITE
            );
            if (first_w_node_ptr == NULL) {
                // No writer exists. No need to wait.
                // Just attach the new record to the end of rw_list and return.
                rwlock->rw_list = append_node(rwlock->rw_list, new_record_ptr);
            } else {
                // At least one writer exists.
                // Attach the new record to the end of rw_list and wait.
                rwlock->rw_list = append_node(rwlock->rw_list, new_record_ptr);
                cond_wait(
                    &(new_record_ptr->rw_lock_permission),
                    &(rwlock->rw_list_manipulation)
                );
            }
            break;
        }
        case RWLOCK_WRITE: {
            if (rwlock->rw_list == NULL) {
                // No reader or writer exists. No need to wait.
                // Just attach the new record to the end of rw_list and return.
                rwlock->rw_list = append_node(rwlock->rw_list, new_record_ptr);
            } else {
                // At least one reader or writer exists.
                
                // insert the new record
                list_node *first_w_node_ptr = find_by_rw_lock_type(
                    rwlock->rw_list,
                    RWLOCK_WRITE
                );
                if (first_w_node_ptr == NULL) {
                    append_node(rwlock->rw_list, new_record_ptr);
                } else {
                    // In this case, any writer must be placed at
                    // the beginning of rw_list.

                    list_node *first_r_node_ptr = find_by_rw_lock_type(
                        rwlock->rw_list,
                        RWLOCK_READ
                    );
                    if (first_r_node_ptr == NULL) {
                        append_node(rwlock->rw_list, new_record_ptr);
                    } else {
                        // place the new record as the last writer
                        append_node(first_r_node_ptr, new_record_ptr);
                    }
                }

                // wait
                cond_wait(
                    &(new_record_ptr->rw_lock_permission),
                    &(rwlock->rw_list_manipulation)
                );
            }
            break;
        }
        default: {
            break;
        }
    }    
    mutex_unlock(&(rwlock->rw_list_manipulation));
}

/**
 * @brief Indicates that the calling thread is done using the locked state 
 * in whichever mode it was granted access for.
 *
 * @param rwlock A pointer to a rwlock struct
 */
void rwlock_unlock(rwlock_t *rwlock) {
    // do argument checking
    if (rwlock == NULL) {
        return;
    }

    int my_thread_id = thr_getid();

    mutex_lock(&(rwlock->rw_list_manipulation));

    // find the corresponding node from rw_list and free resources,
    // i.e., list_node, rw_record, rw_lock_permission
    list_node *my_node_ptr = find_by_thread_id(rwlock->rw_list, my_thread_id);
    if (my_node_ptr == NULL) {
        mutex_unlock(&(rwlock->rw_list_manipulation));
        return;
    }
    rw_record *my_record_ptr = my_node_ptr->data;
    int my_rw_lock_type = my_record_ptr->rw_lock_type;
    cond_destroy(&(my_record_ptr->rw_lock_permission));
    free(my_record_ptr);
    rwlock->rw_list = remove_node(rwlock->rw_list, my_node_ptr);

    // see if anyone else can get a lock
    
    switch (my_rw_lock_type) {
        case RWLOCK_READ: {
            if (rwlock->rw_list != NULL) {
                rw_record *first_record_ptr = rwlock->rw_list->data;
                if (first_record_ptr->rw_lock_type == RWLOCK_WRITE) {
                    cond_signal(&(first_record_ptr->rw_lock_permission));
                }
            }
        }
        case RWLOCK_WRITE: {
            if (rwlock->rw_list != NULL) {
                rw_record *first_record_ptr = rwlock->rw_list->data;
                if (first_record_ptr->rw_lock_type == RWLOCK_WRITE) {
                    // grant a writer lock
                    cond_signal(&(first_record_ptr->rw_lock_permission));
                } else {
                    // grant one or several reader locks
                    // (till the first writer lock requestor)
                    list_node *node_ptr = rwlock->rw_list;
                    do {
                        rw_record *record_ptr = node_ptr->data;
                        if (record_ptr->rw_lock_type == RWLOCK_WRITE) {
                            break;
                        }
                        cond_signal(&(record_ptr->rw_lock_permission));
                        node_ptr = node_ptr->next;
                    } while (node_ptr != rwlock->rw_list);
                }
            }
        }
        default: {
            break;
        }
    }    

    mutex_unlock(&(rwlock->rw_list_manipulation));
}

/**
 * @brief Blocks the calling thread until it has been granted 
 * the requested form of access
 * 
 * When the function returns: no threads hold the lock in RWLOCK WRITE mode; 
 * the invoking thread, and possibly some other threads, hold the lock 
 * in RWLOCK READ mode; previously blocked or newly arriving 
 * writers must still wait for the 
 * lock to be released entirely.
 *
 * @param rwlock A pointer to a rwlock struct
 */
void rwlock_downgrade(rwlock_t *rwlock) {
    // do argument checking
    if (rwlock == NULL) {
        return;
    }

    int my_thread_id = thr_getid();

    mutex_lock(&(rwlock->rw_list_manipulation));

    // verify the caller is holding a writer lock
    if (rwlock->rw_list == NULL) {
        mutex_unlock(&(rwlock->rw_list_manipulation));
        return;
    }
    rw_record *first_record_ptr = rwlock->rw_list->data;
    if (
        first_record_ptr->thread_id != my_thread_id ||
        first_record_ptr->rw_lock_type != RWLOCK_WRITE
    ) {
        mutex_unlock(&(rwlock->rw_list_manipulation));
        return;
    }    

    // If any, grant a reader lock to each reader lock requestor
    // and move writer lock requestors' nodes to the end of rw_list.
    // Note that the current thread has not been downgraded to a reader.
    list_node *first_reader_node_ptr = find_by_rw_lock_type(
        rwlock->rw_list,
        RWLOCK_READ
    );
    if (first_reader_node_ptr != NULL) {
        list_node *node_ptr = first_reader_node_ptr;
        do {
            rw_record *record_ptr = node_ptr->data;
            cond_signal(&(record_ptr->rw_lock_permission));
            node_ptr = node_ptr->next;
        } while (node_ptr != rwlock->rw_list);

        node_ptr = rwlock->rw_list->next;
        while (((rw_record *)(node_ptr->data))->rw_lock_type == RWLOCK_WRITE) {
            rw_record *record_ptr = node_ptr->data;
            remove_node(rwlock->rw_list, node_ptr);
            append_node(rwlock->rw_list, record_ptr);
        }
    }

    // downgrade
    first_record_ptr->rw_lock_type = RWLOCK_READ;

    mutex_unlock(&(rwlock->rw_list_manipulation));
}

/**
 * @brief Deactivates the lock pointed to by rwlock.
 * 
 * It is illegal for an application to use a readers/writers lock after 
 * it has been destroyed (unless 
 * and until it is later re-initialized). It is illegal for an application 
 * to invoke rwlock destroy() 
 * on a lock while the lock is held or while threads are waiting on it.
 *
 * @param rwlock A pointer to a rwlock struct
 *
 */
void rwlock_destroy(rwlock_t *rwlock) {
    // do argumen checking
    if (rwlock == NULL) {
        return;
    }

    // destruct list_node, rw_record, rw_lock_permission
    // for each reader/writer thread
    mutex_lock(&(rwlock->rw_list_manipulation));
    while (rwlock->rw_list != NULL) {
        rw_record *record_ptr = rwlock->rw_list->data;
        cond_destroy(&(record_ptr->rw_lock_permission));
        free(record_ptr);
        rwlock->rw_list = remove_node(rwlock->rw_list, rwlock->rw_list);
    }
    mutex_unlock(&(rwlock->rw_list_manipulation));
    
    // destruct outermost rw_list_manipulation
    mutex_destroy(&(rwlock->rw_list_manipulation));
}

bool is_rw_lock_type(int type) {
    switch (type) {
        case RWLOCK_READ:
        case RWLOCK_WRITE:
            return true;
    }

    return false;
}

list_node *find_by_rw_lock_type(list_node *rw_list, int rw_lock_type) {
    if (rw_list != NULL) {
        list_node *node_ptr = rw_list;
        do {
            rw_record *record_ptr = node_ptr->data;
            if (record_ptr->rw_lock_type == rw_lock_type) {
                return node_ptr;
            }
            node_ptr = node_ptr->next;
        } while (node_ptr != rw_list);
    }
    return NULL;
}

list_node *find_by_thread_id(list_node *rw_list, int thread_id) {
    if (rw_list != NULL) {
        list_node *node_ptr = rw_list;
        do {
            rw_record *record_ptr = node_ptr->data;
            if (record_ptr->thread_id == thread_id) {
                return node_ptr;
            }
            node_ptr = node_ptr->next;
        } while (node_ptr != rw_list);
    }
    return NULL;
}
