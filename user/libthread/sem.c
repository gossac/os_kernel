/**
 * @file sem.c
 * @brief implementations of the basic semaphore functions
 * 
 * @author Tony Xi (xiaolix)
 * @author Zekun Ma (zekun)
 * @bug No known bug.
 */

#include <syscall.h>
#include <sem.h>
#include <stddef.h>


/**
 * @brief Initialize the semaphore pointed to by sem to the value count.
 * 
 * It is illegal for an application to use a semaphore 
 * before it has been initialized or to initialize one when it is already 
 * initialized and in use.
 *
 * @param sem A pointer to a sem struct
 * @param count Number of counts to put into semaphore
 * @return 0 on success, -1 otherwise
 *
 */
int sem_init( sem_t *sem, int count ){
    if(sem==NULL)   return -1;
    sem->count = count;
    if(mutex_init(&(sem->lock)))  return -1;
    if(cond_init(&(sem->cond)))    return -1;
    return 0;
}

/**
 * @brief Allows a thread to decrement a semaphore value, and may 
 * cause it to block indfinitely until it is legal to perform the
 * decrement.
 *
 * @param sem A pointer to a sem struct
 *
 */
void sem_wait( sem_t *sem ){
    if(sem==NULL)   return;
    mutex_lock(&(sem->lock));

    // decrement count first
    sem->count -= 1;

    // If count is below zero, wait once to get awaken
    if(sem->count < 0){
        cond_wait(&(sem->cond), &(sem->lock));
    }
    mutex_unlock(&(sem->lock));
}

/**
 * @brief Wake up a thread waiting on the semaphore pointed to by sem if one exists
 * and should update the semaphore value regardless.
 *
 * @param sem A pointer to a sem struct
 *
 */
void sem_signal( sem_t *sem ){
    if(sem==NULL)   return;
    mutex_lock(&(sem->lock));

    // signal a thread if count is negative
    if(sem->count < 0){
        cond_signal(&(sem->cond));
    }

    // increment count afterwards
    sem->count += 1;
    mutex_unlock(&(sem->lock));
}

/**
 * @brief Deactivate the semaphore pointed to by sem. 
 * 
 * It is illegal for an application to use a semaphore 
 * after it has been destroyed (unless and until it is later re-initialized).
 *
 * @param sem A pointer to a sem struct
 *
 */
void sem_destroy( sem_t *sem ){
    if(sem==NULL)   return;
    mutex_destroy(&(sem->lock));
    cond_destroy(&(sem->cond));
}