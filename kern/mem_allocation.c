/**
 * @file mem_allocation.c
 * @author Tony Xi (xiaolix)
 * @author Zekun Ma (zekunm)
 * @brief auxiliaries for implementing malloc wrappers
 */

#include <mem_allocation.h> // mem_allocation_lock
#include <mutex.h> // mutex_init

// the lock guarding execution of malloc family
mutex_t mem_allocation_lock;

/**
 * @brief initialize malloc wrappers
 * 
 * @return A negative value on failure, 0 otherwise.
 */
int initialize_mem_allocation(void) {
    if (mutex_init(&mem_allocation_lock) < 0) {
        return -1;
    }
    return 0;
}
