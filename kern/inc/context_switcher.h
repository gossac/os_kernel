/**
 * @file context_switcher.h
 * @author Tony Xi (xiaolix)
 * @author Zekun Ma (zekunm)
 * @brief The prototype of switch_context function.
 */

#ifndef CONTEXT_SWITCHER_H_SEEN
#define CONTEXT_SWITCHER_H_SEEN

#include <ctrl_blk.h> // tcb_t

int switch_context(
    tcb_t *target_tcb_ptr,
    int state,
    blocking_detail_t *blocking_detail_ptr
);

#endif // CONTEXT_SWITCHER_H_SEEN
