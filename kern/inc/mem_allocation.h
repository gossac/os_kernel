/**
 * @file mem_allocation.h
 * @author Tony Xi (xiaolix)
 * @author Zekun Ma (zekunm)
 * @brief auxiliaries for implementing malloc wrappers
 */

#ifndef MEM_ALLOCATION_H_SEEN
#define MEM_ALLOCATION_H_SEEN

#include <mutex.h> // mutex_t

mutex_t mem_allocation_lock;

int initialize_mem_allocation(void);

#endif // MEM_ALLOCATION_H_SEEN
