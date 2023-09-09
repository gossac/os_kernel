/**
 * @file context.h
 * @author Tony Xi (xiaolix)
 * @author Zekun Ma (zekunm)
 * @brief helper functions for saving and loading context
 */

#ifndef CONTEXT_H_SEEN
#define CONTEXT_H_SEEN

#include <ureg.h>
#include <ctrl_blk.h>

/**
 * @brief copy ureg over to the beginning of the kernel stack specified
 *        by the TCB and save the stack pointer in TCB
 * 
 * @param new_tcb_ptr pointer to the TCB
 * @param ureg_ptr where ureg should be copied from
 */
void save_ureg(tcb_t *new_tcb_ptr, ureg_t *ureg_ptr);
/**
 * @brief save context into one TCB and load context from the other TCB
 * 
 * @param original_tcb_ptr pointer to the TCB to be written to
 * @param target_tcb_ptr pointer to the TCB to be read from
 */
void save_and_load(tcb_t *original_tcb_ptr, tcb_t *target_tcb_ptr);

#endif // CONTEXT_H_SEEN
