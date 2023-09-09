/**
 * @file scheduler.h
 * @author Tony Xi (xiaolix)
 * @author Zekun Ma (zekunm)
 * @brief scheduling functions that decide which thread
 *        to run next
 */

#ifndef SCHEDULER_H_SEEN
#define SCHEDULER_H_SEEN

#include <ctrl_blk.h> // tcb_t

tcb_t *round_robin(void);
tcb_t *find_next_thread(void);

#endif // SCHEDULER_H_SEEN
