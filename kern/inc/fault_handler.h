/**
 * @file fault_handler.h
 * @author Tony Xi (xiaolix)
 * @author Zekun Ma (zekunm)
 * @brief handlers to deal with faults
 */

#ifndef FAULT_HANDLER_H_SEEN
#define FAULT_HANDLER_H_SEEN

#include <ureg.h> // ureg_t

void handle_page_fault(ureg_t *ureg_ptr);
void handle_seg_fault(ureg_t *ureg_ptr);
void handle_div_zero_fault(ureg_t *ureg_ptr);
void fault_kill_thread(void);

#endif /* FAULT_HANDLER_H_SEEN */
