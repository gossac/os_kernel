/**
 * @file keyboard.c
 * @author Tony Xi (xiaolix)
 * @author Zekun Ma (zekunm)
 * @brief Implementation of keyboard driver.
 */

#include <keyboard.h> // install_keyboard
#include <simics.h> // lprintf
#include <interrupt.h> // add_interrupt_gate
#include <keyhelp.h> // KEY_IDT_ENTRY
#include <handler_wrapper.h> // wrap_handler33
#include <interrupt_defines.h> // INT_CTL_PORT
#include <asm.h> // outb
#include <stddef.h> // NULL
#include <ctrl_blk.h> // thread_lists
#include <context_switcher.h> // switch_context

/**
 * @brief The scancode buffer which the keyboard interrupt handler
 *        feeds with scancodes that the exposed APIs later
 *        consumes.
 * 
 * This buffer should be manipulated via push_tail and pop_head.
 */
buf_t scancode_buf = {.start_idx = 0};

void handle_keyboard(ureg_t *ureg_ptr);

/**
 * @brief install keyboard driver
 */
void install_keyboard(void) {
    handler_array[KEY_IDT_ENTRY] = handle_keyboard;
    add_interrupt_gate(KEY_IDT_ENTRY, wrap_handler33, KERNEL_PL);
    lprintf("The keyboard has been installed.");
}

/**
 * @brief keyboard interrupt handler
 * 
 * @param ureg_ptr When the interrupt handler starts, ureg_ptr stores the
 *                 register values that record the state of the kernel
 *                 before the interrupt happens. It will also be used to
 *                 restore the registers after the interrupt handler ends.
 *                 So changes on them will reveal after the interrupt is
 *                 handled.
 */
void handle_keyboard(ureg_t *ureg_ptr) {
    char scancode = inb(KEYBOARD_PORT);
    push_tail(&scancode_buf, scancode);

    outb(INT_CTL_PORT,  INT_ACK_CURRENT);

    // wake up the first reader if any
    if (
        thread_lists[READLINE] != NULL &&
        thread_lists[READLINE]->data->blocking_detail.first_reader
    ) {
        switch_context(thread_lists[READLINE]->data, READY_STATE, NULL);
    } 
}

/**
 * @brief push an element into the buffer as the new tail
 * 
 * @param buf_ptr pointer to the buffer
 * @param element the element
 * @return a negative value on failure, 0 otherwise
 */
int push_tail(buf_t *buf_ptr, char element) {
    if (buf_ptr == NULL) {
        return -1;
    }
    if (buf_ptr->element_count >= BUF_LEN) {
        return -1;
    }
    int element_idx = (buf_ptr->start_idx + buf_ptr->element_count) % BUF_LEN;
    buf_ptr->array[element_idx] = element;
    buf_ptr->element_count++;
    return 0;
}

/**
 * @brief pop the element which is the head of the buffer
 * 
 * @param buf_ptr pointer to the buffer
 * @param element_ptr where the element will be stored
 * @return a negative value on failure, 0 otherwise
 */
int pop_head(buf_t *buf_ptr, char *element_ptr) {
    if (buf_ptr == NULL || element_ptr == NULL) {
        return -1;
    }
    if (buf_ptr->element_count <= 0) {
        return -1;
    }
    buf_ptr->element_count--;
    *element_ptr = buf_ptr->array[buf_ptr->start_idx];
    buf_ptr->start_idx = (buf_ptr->start_idx + 1) % BUF_LEN;
    return 0;
}

/**
 * @brief pop the element which is the tail of the buffer
 * 
 * @param buf_ptr pointer to the buffer
 * @param element_ptr where the element will be stored
 * @return a negative value on failure, 0 otherwise
 */
int pop_tail(buf_t *buf_ptr, char *element_ptr) {
    if (buf_ptr == NULL || element_ptr == NULL) {
        return -1;
    }
    if (buf_ptr->element_count <= 0) {
        return -1;
    }
    buf_ptr->element_count--;
    int element_idx = (buf_ptr->start_idx + buf_ptr->element_count) % BUF_LEN;
    *element_ptr = buf_ptr->array[element_idx];
    return 0;
}

/**
 * @brief consume and parse scancodes stored in the buffer until a
 *        character is generated
 * 
 * This function will not wait until there are plenty of scancodes.
 * Neither will it try to parse more scancodes after a character
 * has been generated. This function should be called only when
 * interrupts are disabled.
 * 
 * @param scancode_buf_ptr pointer to the scancode buffer
 * @param ch_ptr where the generated character will be stored
 * @return A negative value on failure, in which case scancodes
 *         may still have been consumed. 0 on success, only
 *         in which case the memory pointed to by ch_ptr is
 *         modified.
 */
int extract_ch(buf_t *scancode_buf_ptr, char *ch_ptr) {
    if (scancode_buf_ptr == NULL || ch_ptr == NULL) {
        return -1;
    }
    char scancode;
    while (!(pop_head(scancode_buf_ptr, &scancode) < 0)) {
        kh_type augmented_ch = process_scancode(scancode);
        if (KH_ISMAKE(augmented_ch) && KH_HASDATA(augmented_ch)) {
            *ch_ptr = KH_GETCHAR(augmented_ch);
            return 0;
        }
    }
    return -1;
}
