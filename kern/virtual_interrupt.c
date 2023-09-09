/**
 * @file virtual_interrupt.c
 * @author Tony Xi (xiaolix)
 * @author Zekun Ma (zekunm)
 * @brief The initializer, dispatcher and handlers for virtual interrupts.
 */

#include <virtual_interrupt.h> // initialize_virtual_interrupt
#include <interrupt.h> // add_trap_gate
#include <hvcall_int.h> // HV_INT
#include <handler_wrapper.h> // wrap_handler222
#include <simics.h> // lprintf
#include <stdbool.h> // true
#include <ureg.h> // ureg_t
#include <stddef.h> // NULL
#include <stdint.h> // uint32_t
#include <ctrl_blk.h> // thread_lists
#include <keyhelp.h> // KEY_IDT_ENTRY
#include <timer_defines.h> // TIMER_IDT_ENTRY
#include <common_kern.h> // USER_MEM_START
#include <page.h> // PAGE_SIZE
#include <vm.h> // USER_PAGE_START
#include <hvcall.h> // GUEST_CRASH_STATUS
#include <asm.h> // outb
#include <interrupt_defines.h> // INT_CTL_PORT
#include <segmentation.h> // SEGSEL_GUEST_KERNEL_CS
#include <keyboard.h> // scancode_buf
#include <cr.h> // get_cr3
#include <loader.h> // push_val
#include <eflags.h> // EFL_IF
#include <idt.h> // IDT_GP
#include <system_call.h> // output_lock
#include <console.h> // putbytes
#include <loader.h> // GUEST_MEM_SIZE

// Length of hvcall_handler_array.
// Please note that depending on the hypervisor call numbers, this macro
// may not be the total count of hypervisor calls available to guests.
#define HVCALL_HANDLER_ARRAY_LEN (HV_PRINT_AT_OP + 1)

// to hold hypervisor call handlers
void (*hvcall_handler_array[HVCALL_HANDLER_ARRAY_LEN])(ureg_t *ureg_ptr) = {
    NULL
};

bool is_host_writable(uint32_t addr, uint32_t size);
bool is_host_readable(uint32_t addr, uint32_t size);
int alter_page_dir(pde_t *page_dir, bool kernel_mode);
void handle_hv_magic(ureg_t *ureg_ptr);
void handle_hv_disable_interrupts(ureg_t *ureg_ptr);
void handle_hv_enable_interrupts(ureg_t *ureg_ptr);
void handle_hv_setidt(ureg_t *ureg_ptr);
void handle_hv_iret(ureg_t *ureg_ptr);
void handle_hv_print(ureg_t *ureg_ptr);
void handle_hv_cons_set_term_color(ureg_t *ureg_ptr);
void handle_hv_cons_set_cursor_pos(ureg_t *ureg_ptr);
void handle_hv_cons_get_cursor_pos(ureg_t *ureg_ptr);
void handle_hv_print_at(ureg_t *ureg_ptr);
void handle_hv_exit(ureg_t *ureg_ptr);
void handle_hv_setpd(ureg_t *ureg_ptr);
void handle_hv_adjustpg(ureg_t *ureg_ptr);

/**
 * @brief prepare for handling virtual interrupts
 */
void initialize_virtual_interrupt(void) {
    // enable trapping into a hypervisor call from user mode
    add_trap_gate(HV_INT, wrap_handler222, USER_PL);

    hvcall_handler_array[HV_MAGIC_OP] = handle_hv_magic;
    hvcall_handler_array[HV_EXIT_OP] = handle_hv_exit;
    hvcall_handler_array[HV_IRET_OP] = handle_hv_iret;
    hvcall_handler_array[HV_SETIDT_OP] = handle_hv_setidt;
    hvcall_handler_array[HV_DISABLE_OP] = handle_hv_disable_interrupts;
    hvcall_handler_array[HV_ENABLE_OP] = handle_hv_enable_interrupts;
    hvcall_handler_array[HV_PRINT_OP] = handle_hv_print;
    hvcall_handler_array[HV_SET_COLOR_OP] = handle_hv_cons_set_term_color;
    hvcall_handler_array[HV_SET_CURSOR_OP] = handle_hv_cons_set_cursor_pos;
    hvcall_handler_array[HV_GET_CURSOR_OP] = handle_hv_cons_get_cursor_pos;
    hvcall_handler_array[HV_PRINT_AT_OP] = handle_hv_print_at;
    hvcall_handler_array[HV_SETPD_OP] = handle_hv_setpd;
    hvcall_handler_array[HV_ADJUSTPG_OP] = handle_hv_adjustpg;
}

/**
 * @brief handle different kinds of virtual interrupts
 * 
 * @param ureg_ptr A collection of register values snapshotted before the
 *                 interrupt happens.
 */
void handle_virtual_interrupt(ureg_t *ureg_ptr) {
    // if the interrupt is a hypervisor call and the hypervisor call handler
    // array has a handler for the call number
    //     run the handler
    //     return
    // if the interrupt is a hardware interrupt
    //     acknowledge the interrupt
    //     if virtual interrupts are disabled
    //         return
    // if the virtual IDT has a handler for the interrupt
    //     deliver the interrupt to the handler
    //     return
    // lprintf("Interrupt %d is not handled for the guest.", interrupt)

    unsigned int interrupt = ureg_ptr->cause;
    pcb_t *current_pcb_ptr = thread_lists[RUNNING_STATE]->data->pcb_ptr;
    if (interrupt == HV_INT) {
        unsigned int call_num = ureg_ptr->eax;
        if (
            call_num < HVCALL_HANDLER_ARRAY_LEN &&
            hvcall_handler_array[call_num] != NULL
        ) {
            hvcall_handler_array[call_num](ureg_ptr);
            return;
        }
    }
    
    if (interrupt == TIMER_IDT_ENTRY) {
        handler_array[interrupt](ureg_ptr);
        if (!current_pcb_ptr->guest_resource.interrupt_enable_flag) {
            return;
        }
    }

    kh_type augmented_ch;
    if (interrupt == KEY_IDT_ENTRY) {
        char scancode = inb(KEYBOARD_PORT);
        push_tail(&scancode_buf, scancode);
        outb(INT_CTL_PORT,  INT_ACK_CURRENT);
        pop_head(&scancode_buf, &scancode);
        augmented_ch = process_scancode(scancode);
        if (!current_pcb_ptr->guest_resource.interrupt_enable_flag) {
            return;
        }
    }

    if (
        interrupt < VIRTUAL_IDT_LEN &&
        current_pcb_ptr->guest_resource.virtual_idt[interrupt] !=
            (uint32_t)NULL
    ) {
        // Delivery of virtual interrupts is suspended.
        uint32_t eflags = ureg_ptr->eflags;
        if (current_pcb_ptr->guest_resource.interrupt_enable_flag) {
            eflags |= EFL_IF;
        } else {
            eflags &= ~EFL_IF;
        }
        current_pcb_ptr->guest_resource.interrupt_enable_flag = false;

        // If the guest is not already in guest kernel mode, then... 
        // Otherwise...
        uint32_t esp;
        if (ureg_ptr->cs != SEGSEL_GUEST_KERNEL_CS) {
            esp = current_pcb_ptr->guest_resource.esp0;
            uint32_t magic_num = GUEST_INTERRUPT_UMODE;
            if (
                push_val(&esp, &(ureg_ptr->esp), sizeof(ureg_ptr->esp)) < 0 ||
                push_val(&esp, &eflags, sizeof(eflags)) < 0 ||
                push_val(&esp, &magic_num, sizeof(magic_num)) < 0
            ) {
                lprintf("esp %lx passed into host is invalid, crashing the guest", esp);
                crash_guest();
                return;
            }
        } else {
            esp = ureg_ptr->esp + USER_MEM_START;
            uint32_t magic_num = GUEST_INTERRUPT_KMODE;
            if (
                push_val(&esp, &eflags, sizeof(eflags)) < 0 ||
                push_val(&esp, &magic_num, sizeof(magic_num))
            ) {
                lprintf("esp %lx passed into host is invalid, crashing the guest", esp);
                crash_guest();
                return;
            }
        }

        // Regardless of whether or not there was a stack switch...
        uint32_t err_code = 0;
        if (interrupt == IDT_GP || interrupt == IDT_PF) {
            err_code = ureg_ptr->error_code;
        } else {
            if (interrupt == KEY_IDT_ENTRY) {
                err_code = augmented_ch;
            }
        }
        uint32_t faulting_addr = 0;
        if (interrupt == IDT_PF) {
            faulting_addr = ureg_ptr->cr2 - USER_MEM_START;
        }
        if (
            push_val(&esp, &(ureg_ptr->eip), sizeof(ureg_ptr->eip)) < 0 ||
            push_val(&esp, &(err_code), sizeof(err_code)) < 0 ||
            push_val(&esp, &(faulting_addr), sizeof(faulting_addr)) < 0
        ) {
            lprintf("esp %lx passed into host is invalid, crashing the guest", esp);
            crash_guest();
            return;
        }

        // Note that when a guest switches from guest kernel mode to guest
        // user mode, which pages it has permission to access must change
        // accordingly.
        uint32_t cr3 = get_cr3();
        pde_t *page_dir = (pde_t *)((cr3 >> PAGE_SHIFT) << PAGE_SHIFT);
        alter_page_dir(page_dir, true);
        set_cr3(cr3);

        ureg_ptr->ss = SEGSEL_GUEST_KERNEL_DS;
        ureg_ptr->esp = esp - USER_MEM_START;
        ureg_ptr->cs = SEGSEL_GUEST_KERNEL_CS;
        ureg_ptr->eip =
            current_pcb_ptr->guest_resource.virtual_idt[interrupt] -
            USER_MEM_START;
        ureg_ptr->gs = SEGSEL_GUEST_KERNEL_DS;
        ureg_ptr->fs = SEGSEL_GUEST_KERNEL_DS;
        ureg_ptr->es = SEGSEL_GUEST_KERNEL_DS;
        ureg_ptr->ds = SEGSEL_GUEST_KERNEL_DS;
        return;
    }

    lprintf("Interrupt %u caused a PebPeb triple fault.", interrupt);
    crash_guest();
}

/**
 * @brief Alters the us bit in each pte according to the permission level
 *        This is used to change page permissions accordingly when switching
 *        between guest kernel and guest user mode  
 * 
 * @param page_dir address of the page directory
 * @param kernel_mode Permission level of the the page directory 
 * 
 * @return 0 on success and -1 otherwise
 */
int alter_page_dir(pde_t *page_dir, bool kernel_mode) {
    if (page_dir == NULL) {
        return -1;
    }
    for (
        uint32_t addr = USER_PAGE_START;
        addr < USER_MEM_START + USER_MEM_START;
        addr += PAGE_SIZE
    ) {
        uint32_t pde_idx = (addr >> PAGE_SHIFT) / PTE_COUNT;
        if (page_dir[pde_idx].p != 1) {
            return -1;
        }
        pte_t *page_table = (pte_t *)(page_dir[pde_idx].pt_addr << PAGE_SHIFT);
        uint32_t pte_idx = (addr >> PAGE_SHIFT) % PTE_COUNT;
        if (page_table[pte_idx].p != 1) {
            return -1;
        }
        page_table[pte_idx].us = kernel_mode ? 1 : 0;
    }
    return 0;
}

/**
 * @brief crash the guest
 * 
 * "Crashing the guest" means pretending the guest had invoked
 * hv_exit(GUEST_CRASH_STATUS).
 */
void crash_guest(void) {
    void *arg_array[] = {(void *)GUEST_CRASH_STATUS};
    ureg_t ureg = {.esp = (uint32_t)arg_array - USER_MEM_START};
    handle_hv_exit(&ureg);
}

void handle_hv_magic(ureg_t *ureg_ptr) {
    ureg_ptr->eax = HV_MAGIC;
}

void handle_hv_disable_interrupts(ureg_t *ureg_ptr) {
    pcb_t *current_pcb_ptr = thread_lists[RUNNING_STATE]->data->pcb_ptr;
    current_pcb_ptr->guest_resource.interrupt_enable_flag = false;
}

void handle_hv_enable_interrupts(ureg_t *ureg_ptr) {
    pcb_t *current_pcb_ptr = thread_lists[RUNNING_STATE]->data->pcb_ptr;
    current_pcb_ptr->guest_resource.interrupt_enable_flag = true;
}

void handle_hv_setidt(ureg_t *ureg_ptr) {
    void **arg_array = (void **)(ureg_ptr->esp + USER_MEM_START);
    int irqno = (int)arg_array[0];
    void *eip = arg_array[1];

    if (irqno >= VIRTUAL_IDT_LEN) {
        return;
    }

    pcb_t *current_pcb_ptr = thread_lists[RUNNING_STATE]->data->pcb_ptr;
    if (eip == NULL) {
        current_pcb_ptr->guest_resource.virtual_idt[irqno] = (uint32_t)NULL;
    } else {
        current_pcb_ptr->guest_resource.virtual_idt[irqno] = (uint32_t)eip +
            USER_MEM_START;
    }
}

void handle_hv_iret(ureg_t *ureg_ptr) {
    void **arg_array = (void **)(ureg_ptr->esp + USER_MEM_START);
    void *eip = arg_array[0];
    unsigned int eflags = (unsigned int)arg_array[1];
    void *esp = arg_array[2];
    void *esp0 = arg_array[3];
    unsigned int eax = (unsigned int)arg_array[4];

    // Simplification: crash if esp0 is not 0
    if (esp0 != NULL || !check_eflags(ureg_ptr->eflags, eflags)) {
        lprintf("esp0 %p passed into hv_iret is not zero, crashing the guest", esp0);
        crash_guest();
        return;
    }

    // Alter the registers to the values specified 
    ureg_ptr->eip = (uint32_t)eip;
    ureg_ptr->eflags = eflags | EFL_IF;
    pcb_t *current_pcb_ptr = thread_lists[RUNNING_STATE]->data->pcb_ptr;
    current_pcb_ptr->guest_resource.interrupt_enable_flag =
        ((eflags & EFL_IF) != 0);
    ureg_ptr->esp = (uint32_t)esp;
    ureg_ptr->eax = eax;
}

void handle_hv_print(ureg_t *ureg_ptr) {
    void **arg_array = (void **)(ureg_ptr->esp + USER_MEM_START);
    int len = (int)arg_array[0];
    char *buf = arg_array[1] + USER_MEM_START;

    if (
        len < 0 || 
        len > HV_PRINT_MAX || 
        !is_host_readable((uint32_t)buf, len)
    ) {
        lprintf("len of %d passed into hv_print is invalid, crashing the guest", len);
        crash_guest();
        return;
    }

    mutex_lock(&output_lock);
    putbytes(buf, len);
    mutex_unlock(&output_lock);
}

void handle_hv_cons_set_term_color(ureg_t *ureg_ptr) {
    void **arg_array = (void **)(ureg_ptr->esp + USER_MEM_START);
    int color = (int)arg_array[0];
    mutex_lock(&output_lock);
    bool success = !(set_term_color(color) < 0);
    mutex_unlock(&output_lock);
    if (!success) {
        lprintf("set term color failed, crashing the guest");
        crash_guest();
    }
}

void handle_hv_cons_set_cursor_pos(ureg_t *ureg_ptr) {
    void **arg_array = (void **)(ureg_ptr->esp + USER_MEM_START);
    int row = (int)arg_array[0];
    int col = (int)arg_array[1];
    mutex_lock(&output_lock);
    bool success = !(set_cursor(row, col) < 0);
    mutex_unlock(&output_lock);
    if (!success) {
        lprintf("set cursor failed, crashing the guest");
        crash_guest();
    }
}

void handle_hv_cons_get_cursor_pos(ureg_t *ureg_ptr) {
    void **arg_array = (void **)(ureg_ptr->esp + USER_MEM_START);
    int *row = arg_array[0] + USER_MEM_START;
    int *col = arg_array[1] + USER_MEM_START;
    if (
        !is_host_writable((uint32_t)row, sizeof(int)) ||
        !is_host_writable((uint32_t)col, sizeof(int))
    ) {
        lprintf("row %p and column %p pointers invalid , crashing the guest", row, col);
        crash_guest();
        return;
    }
    mutex_lock(&output_lock);
    get_cursor(row, col);
    mutex_unlock(&output_lock);
}

void handle_hv_print_at(ureg_t *ureg_ptr) {
    // grab arguments
    void **arg_array = (void **)(ureg_ptr->esp + USER_MEM_START);
    int len = (int)arg_array[0];
    char *buf = arg_array[1] + USER_MEM_START;
    int row = (int)arg_array[2];
    int col = (int)arg_array[3];
    int color = (int)arg_array[4];

    // check if buf can be read
    if (
        len < 0 || 
        len > HV_PRINT_MAX || 
        !is_host_readable((uint32_t)buf, len)
    ) {
        lprintf("buf %p pointer invalid , crashing the guest", buf);
        crash_guest();
        return;
    }

    // get old cursor and color
    int old_color;
    int old_row;
    int old_col;

    mutex_lock(&output_lock);
    get_cursor(&old_row, &old_col);
    get_term_color(&old_color);
    if (
        // set cursor and color
        set_cursor(row, col) < 0 ||
        set_term_color(color) < 0
    ) {
        set_cursor(old_row, old_col);
        set_term_color(old_color);
        mutex_unlock(&output_lock);

        lprintf("setting cursor and color failed in print_at, crashing the guest");
        crash_guest();
        return;
    }
    // print to console
    putbytes(buf, len);
    // restore old cursor and color
    set_cursor(old_row, old_col);
    set_term_color(old_color);
    mutex_unlock(&output_lock);
}

void handle_hv_exit(ureg_t *ureg_ptr) {
    void **arg_array = (void **)(ureg_ptr->esp + USER_MEM_START);
    int status = (int)arg_array[0];

    pcb_t *current_pcb_ptr = thread_lists[RUNNING_STATE]->data->pcb_ptr;
    current_pcb_ptr->status = status;
    ureg_t ureg = {.cause = 0};
    handle_vanish(&ureg);
}

void handle_hv_setpd(ureg_t *ureg_ptr) {
    lprintf("hv_setpd not implemented, crashing the guest");
    crash_guest();
}

void handle_hv_adjustpg(ureg_t *ureg_ptr) {
    lprintf("hv_adjustpg not implemented, crashing the guest");
    crash_guest();
}

/** @brief Helper method to check if a memory range is writable from the
 * host perspective
 *
 * @return true if the enitre region is writable, false otherwise
 */
bool is_host_writable(uint32_t addr, uint32_t size) {
    if (size > 0) {
        if (!(
            addr >= USER_MEM_START &&
            (uint64_t)addr + (uint64_t)size <= USER_MEM_START + GUEST_MEM_SIZE
        )) {
            return false;
        }

        pde_t *page_dir = (pde_t *)((get_cr3() >> PAGE_SHIFT) << PAGE_SHIFT);
        for (
            uint64_t i = (addr >> PAGE_SHIFT) << PAGE_SHIFT;
            i < (uint64_t)addr + (uint64_t)size;
            i += PAGE_SIZE
        ) {
            uint32_t access;
            get_access(page_dir, i, &access);
            if (access != READ_WRITE) {
                return false;
            }
        }
    }

    return true;
}

/** @brief Helper method to check if a memory range is readable from the
 * host perspective
 *
 * @return true if the enitre region is readable, false otherwise
 */
bool is_host_readable(uint32_t addr, uint32_t size) {
    if (size > 0) {
        if (!(
            addr >= USER_MEM_START &&
            (uint64_t)addr + (uint64_t)size <= USER_MEM_START + GUEST_MEM_SIZE
        )) {
            return false;
        }
    }

    return true;
}
