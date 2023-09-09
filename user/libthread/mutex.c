/**
 * @file mutex.c
 * @brief implementations of the basic mutex functions
 * using the atomic xchg assembly instruction
 * 
 * @author Tony Xi (xiaolix)
 * @author Zekun Ma (zekun)
 * @bug No known bug.
 */

#include <mutex.h>
#include <xchange_stub.h>
#include <stddef.h>
#include <thread.h>
#include <simics.h>

/**
 * @brief This function should initialize the mutex pointed to by
 * mp. 
 * 
 * It is illegal for an application to use a mutex before it 
 * has been initialized or to initialize 
 *
 * @param mp A pointer to a mutex struct
 * @return 0 on success, -1 otherwise
 *
 */
int mutex_init(mutex_t *mp) {
    if (mp == NULL)
        return -1;
    mp->tid = -1;
    mp->lock_state = UNLOCKED;
    return 0;
}

/**
 * @brief This function should deactivate the mutex
 * pointed to by mp. 
 * 
 * It is illegal for an application to use a mutex after it has been destroyed
 * (unless and until it is later re-initialized).
 * It is illegal for an application to attempt to destroy a mutex 
 * while it is locked or threads are trying to acquire it.
 *
 * @param mp A pointer to a mutex struct
 *
 */
void mutex_destroy(mutex_t *mp) {}

/**
 * @brief Atomomically lock the mutex 
 * and block until the lock is obtained.
 * 
 * Ensures mutual exclusion in the region between itself 
 * and a call to mutex_unlock(). A thread calling this function while 
 * another thread is in an interfering critical section 
 * must not proceed until it is able to claim the lock.
 *
 * @param mp A pointer to a mutex struct
 *
 */
void mutex_lock(mutex_t *mp) {
    if (mp == NULL)
        return;

    while (xchange(&(mp->lock_state), LOCKED) != UNLOCKED) {
        thr_yield(mp->tid);
    }

    mp->tid = thr_getid();
}


/**
 * @brief Atomomically unlock the mutex 
 * and allow one other thread to obtain lock
 * 
 * Signals the end of a region of mutual exclusion. The 
 * calling thread gives up its claim to the lock. 
 * It is illegal for an application to unlock a mutex
 * that is not locked.
 *
 * @param mp A pointer to a mutex struct
 *
 */
void mutex_unlock(mutex_t *mp) {
    if (mp == NULL)
        return;

    mp->tid = -1;
    xchange(&(mp->lock_state), UNLOCKED);
}
