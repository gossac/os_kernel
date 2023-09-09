/**
 * @file mutex.c
 * @brief implementation of the basic mutex functions
 *        using the xchg assembly instruction and the
 *        yield system call handler
 * 
 * @author Tony Xi (xiaolix)
 * @author Zekun Ma (zekunm)
 * @bug No known bugs.
 */

#include <mutex.h>
#include <xchange_stub.h>
#include <stddef.h>
#include <system_call.h>
#include <assert.h>
#include <simics.h>
#include <ctrl_blk.h>

#define UNLOCKED    (0)
#define LOCKED      (1)

/**
 * @brief This function should initialize the mutex pointed to
 *        by mp. 
 * 
 * It is illegal for an application to use a mutex before it
 * has been initialized or to initialize one when it is
 * already initialized and in use.
 *
 * @param mp A pointer to the mutex.
 * @return 0 on success, a negative number on failure.
 */
int mutex_init(mutex_t *mp) {
    affirm(mp != NULL);
    mp->lock_state = UNLOCKED;
    mp->tid = -1;
    return 0;
}

/**
 * @brief This function should deactivate the mutex
 *        pointed to by mp. 
 * 
 * It is illegal for an application to use a mutex after it
 * has been destroyed (unless and until it is later
 * re-initialized). It is illegal for an application to
 * attempt to destroy a mutex while it is locked or threads
 * are trying to acquire it.
 *
 * @param mp A pointer to the mutex.
 */
void mutex_destroy(mutex_t *mp) {
    affirm(mp != NULL);
}

/**
 * @brief Atomomically lock the mutex.
 * 
 * Ensures mutual exclusion in the region between itself 
 * and a call to mutex_unlock. A thread calling this
 * function while another thread is in an interfering
 * critical section must not proceed until it is able to
 * claim the lock.
 *
 * @param mp A pointer to the mutex.
 */
void mutex_lock(mutex_t *mp) {
    affirm(mp != NULL);

    // yield if the mutex is locked
    while (xchange(&(mp->lock_state), LOCKED) != UNLOCKED) {
        ureg_t ureg = {.esi = mp->tid};
        handle_yield(&ureg);
    }

    mp->tid = thread_lists[RUNNING_STATE]->data->tid;
}

/**
 * @brief Make an attempt to atomically lock the mutex.
 * 
 * This function will make one attempt and will not block
 * on failure, which includes as two cases:
 *     1. The mutex provided is invalid.
 *     2. The mutex has been locked by others.
 * 
 * @param mp A pointer to the mutex.
 * @return 0 on success, a negative value on failure.
 */
int mutex_try_lock(mutex_t *mp) {
    affirm(mp != NULL);

    if (xchange(&(mp->lock_state), LOCKED) != UNLOCKED) {
        return -1;
    }

    mp->tid = thread_lists[RUNNING_STATE]->data->tid;
    return 0;
}

/**
 * @brief Atomomically unlock the mutex.
 * 
 * Signals the end of a region of mutual exclusion. The 
 * calling thread gives up its claim to the lock. It is
 * illegal for an application to unlock a mutex that is
 * not locked.
 *
 * @param mp A pointer to the mutex.
 */
void mutex_unlock(mutex_t *mp) {
    affirm(mp != NULL);

    // put UNLOCKED into the mutex
    mp->tid = -1;
    mp->lock_state = UNLOCKED;
}
