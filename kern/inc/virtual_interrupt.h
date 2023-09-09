/**
 * @file virtual_interrupt.h
 * @author Tony Xi (xiaolix)
 * @author Zekun Ma (zekunm)
 * @brief functions for dealing with virtual interrupts
 */

#ifndef VIRTUAL_INTERRUPT_H_SEEN
#define VIRTUAL_INTERRUPT_H_SEEN

#include <ureg.h> // ureg_t

void initialize_virtual_interrupt(void);
void handle_virtual_interrupt(ureg_t *ureg_ptr);
void crash_guest(void);

#endif // VIRTUAL_INTERRUPT_H_SEEN
