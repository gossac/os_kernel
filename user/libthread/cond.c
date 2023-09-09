/**
 * @file cond.c
 * @brief implementations of the basic conditional variable functions
 * using the mutex library.
 * 
 * @author Tony Xi (xiaolix)
 * @author Zekun Ma (zekun)
 * @bug No known bug.
 */

#include <syscall.h>
#include <cond.h>
#include <stddef.h>
#include <thread.h>
#include <stdbool.h>

/**
 * @brief Initialize the condition variable pointed to by cv. 
 * 
 * It is illegal for an application to use a condition variable 
 * before it has been initialized or to initialize one when 
 * it is already initialized and in use.
 *
 * @param cv A pointer to a cond struct
 * @return 0 on success, -1 otherwise
 *
 */
int cond_init( cond_t *cv ){
    if(cv == NULL || mutex_init(&(cv->lock)) < 0)    return -1;
    cv->tid_head = NULL;
    return 0;
}

/**
 * @brief Deactivates the condition variable pointed to by cv. 
 * 
 * It is illegal for an application to use a condition variable after it
 * has been destroyed (unless and until it is later re-initialized). 
 * It is illegal for an application to invoke cond destroy() 
 * on a condition variable while threads are blocked waiting on it.
 *
 * @param cv A pointer to a cond struct
 *
 */
void cond_destroy( cond_t *cv ){
    if(cv==NULL)   return;
    while (cv->tid_head != NULL) {
        cv->tid_head = remove_node(cv->tid_head, cv->tid_head);
    }
    mutex_destroy(&(cv->lock));
}

/**
 * @brief Allows a thread to wait for a condition and release 
 * the associated mutex that it needs to hold to check that condition
 * 
 * The calling thread blocks, waiting to be signaled. The blocked thread may
 * be awakened by a cond signal() or a cond broadcast(). Upon return from cond wait(),
 * *mp has been re-acquired on behalf of the calling thread.
 *
 * @param cv A pointer to a cond struct
 * @param mp A pointer to a mutex struct
 *
 */
void cond_wait( cond_t *cv, mutex_t *mp ){
    if(cv==NULL){
        return; 
    }

    // Obtain tid and create node ptr
    int tid = thr_getid();
    list_node *my_node;

    // Lock the cond_t and append an entry to list
    mutex_lock(&(cv->lock));
    cv->tid_head = append_node(cv->tid_head, (void*)tid);
    my_node = cv->tid_head->previous;
    mutex_unlock(&(cv->lock));
    
    // Release mp and wait
    mutex_unlock(mp);
    int reject = 0;
    if(deschedule(&reject) < 0) {
        // Pointer contaminated, remove entry from list
        mutex_lock(&(cv->lock));
        cv->tid_head = remove_node(cv->tid_head, my_node);
        mutex_unlock(&(cv->lock));
    }
    mutex_lock(mp);
}

/**
 * @brief Wake up a thread waiting on the 
 * condition variable pointed to by cv, if one exists.
 *
 * @param cv A pointer to a cond struct
 *
 */
void cond_signal( cond_t *cv ){
    if(cv==NULL)   return;
    
    int tid;
    bool find_thread = false;
    
    // Grab thread info if list is not empty
    mutex_lock(&(cv->lock));
    if(cv->tid_head){
        find_thread = true;
        tid = (int) cv->tid_head->data;
        cv->tid_head = remove_node(cv->tid_head, cv->tid_head);
    }
    mutex_unlock(&(cv->lock));
    
    // Attempt to wake only if thread is in list
    if (find_thread) {

        // Keep trying until thread is awoken
        while (make_runnable(tid))
        {
            // Yield to thread if it hasn't been descheduled yet
            thr_yield(tid);
        }
    }
}

/**
 * @brief Wake up all threads waiting on the condition variable pointed to by cv.
 *
 * should not awaken threads which may invoke cond wait(cv)
 * after" this call to cond broadcast() has begun execution
 * 
 * @param cv A pointer to a cond struct
 *
 */
void cond_broadcast( cond_t *cv ){
    if(cv==NULL)   return;

    list_node *waiting_threads = NULL;
    mutex_lock(&(cv->lock));
    waiting_threads = cv->tid_head;

    // Set list of cv to NULL
    cv->tid_head = NULL;
    mutex_unlock(&(cv->lock));

    // loop until no entry is left in the list
    while (waiting_threads){
        // grab tid of the first entry in list
        int tid = (int) waiting_threads->data;

        // keep trying to wake said thread
        while (make_runnable(tid))
        {
            // yield to thread if it has not been descheduled
            thr_yield(tid);
        }

        // free the resource from detached list  
        waiting_threads = remove_node(waiting_threads, waiting_threads);
    }
}