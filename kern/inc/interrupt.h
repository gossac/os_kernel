/**
 * @file interrupt.h
 * @author Tony Xi (xiaolix)
 * @author Zekun Ma (zekunm)
 * @brief functions for setting up interrupt handlers
 */

#ifndef INTERRUPT_H_SEEN
#define INTERRUPT_H_SEEN

#include <ureg.h> // ureg_t
#include <stdint.h> // uint32_t
#include <idt.h> // IDT_ENTS
#include <eflags.h> // EFL_IOPL_SHIFT

// The user privilege level (3)
#define USER_PL (EFL_IOPL_RING3 >> EFL_IOPL_SHIFT)
// The kernel privilege level (0)
#define KERNEL_PL (EFL_IOPL_RING0 >> EFL_IOPL_SHIFT)

void (*handler_array[IDT_ENTS])(ureg_t *ureg_ptr);

int initialize_idt(void);
void handle(ureg_t *ureg_handler);
void add_interrupt_gate(int interrupt, void (*handler_wrapper)(void), uint32_t dpl);
void add_trap_gate(int interrupt, void (*handler_wrapper)(void), uint32_t dpl);

#endif // INTERRUPT_H_SEEN
