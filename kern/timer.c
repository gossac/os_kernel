/**
 * @file timer.c
 * @author Tony Xi (xiaolix)
 * @author Zekun Ma (zekunm)
 * @brief Implementation of timer driver.
 */

#include <timer.h> // install_timer
#include <timer_defines.h> // TIMER_RATE
#include <interrupt.h> // add_interrupt_gate
#include <simics.h> // lprintf
#include <asm.h> // outb
#include <stddef.h> // NULL
#include <limits.h> // CHAR_BIT
#include <interrupt_defines.h> // INT_ACK_CURRENT
#include <handler_wrapper.h> // wrap_handler32
#include <scheduler.h> // round_robin
#include <context_switcher.h> // switch_context
#include <ctrl_blk.h> // thread_lists

// how many timer interrupts within a second
#define TIMER_INTERRUPT_HZ (500)
// how many context switches triggered by timer interrupts within a second
#define ROUND_ROBIN_HZ (500)

void handle_timer(ureg_t *ureg_ptr);
void register_timer(void (*tickback)(unsigned int));
void start_timer(void);

// callback function that will be invoked every time a timer interrupt comes
void (*callback)(unsigned int) = NULL;
// count of ticks since kernel startup
unsigned int tick_count = 0;

/**
 * @brief timer interrupt handler
 * 
 * @param ureg_ptr When the interrupt handler starts, ureg_ptr stores the
 *                 register values that record the state of the kernel
 *                 before the interrupt happens. It will also be used to
 *                 restore the registers after the interrupt handler ends.
 *                 So changes on them will reveal after the interrupt is
 *                 handled.
 */
void handle_timer(ureg_t *ureg_ptr) {
    tick_count++;
    outb(INT_CTL_PORT, INT_ACK_CURRENT);

    if (callback != NULL) {
        callback(tick_count);
    }

    // we don't take this turn to round robin if there's a sleeping thread
    // to wake up
    if (
        thread_lists[SLEEP] != NULL &&
        thread_lists[SLEEP]->data->blocking_detail.wakeup_time <= tick_count
    ) {
        tcb_t *target_tcb_ptr = thread_lists[SLEEP]->data;
        switch_context(target_tcb_ptr, READY_STATE, NULL);
    } else {
        if (tick_count % (TIMER_INTERRUPT_HZ / ROUND_ROBIN_HZ) == 0) {
            tcb_t *target_tcb_ptr = round_robin();
            if (target_tcb_ptr != NULL) {
                switch_context(target_tcb_ptr, READY_STATE, NULL);
            }
        }
    }
}

void register_timer(void (*tickback)(unsigned int)) {
    callback = tickback;

    handler_array[TIMER_IDT_ENTRY] = handle_timer;
    add_interrupt_gate(TIMER_IDT_ENTRY, wrap_handler32, KERNEL_PL);
}

/**
 * @brief set up the timer device
 */
void start_timer(void) {
    // send the number of timer cycles between interrupts to the timer
    outb(TIMER_MODE_IO_PORT, TIMER_SQUARE_WAVE);
    
    // first send it the least significant byte
    // Is TIMER_RATE applicable?
    int cycle_count = TIMER_RATE / TIMER_INTERRUPT_HZ;
    outb(TIMER_PERIOD_IO_PORT, cycle_count);

    // then the most significant byte
    cycle_count = cycle_count >> CHAR_BIT;
    outb(TIMER_PERIOD_IO_PORT, cycle_count);
}

/**
 * @brief initialize timer
 * 
 * @param tickback the callback function invoked by timer interrupt handler
 */
void install_timer(void (*tickback)(unsigned int)) {
    register_timer(tickback);
    start_timer();
    lprintf("The timer has been installed.");
}
