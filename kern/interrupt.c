/**
 * @file interrupt.c
 * @author Tony Xi (xiaolix)
 * @author Zekun Ma (zekunm)
 * @brief functions for setting up interrupt handlers
 */

#include <asm.h> // idt_base
#include <stdint.h> // uint16_t
#include <limits.h> // CHAR_BIT
#include <interrupt.h> // handle
#include <simics.h> // lprintf
#include <seg.h> // SEGSEL_KERNEL_CS
#include <stddef.h> // NULL
#include <idt.h> // IDT_ENTS
#include <handler_wrapper.h> // wrap_user_provided_handler
#include <ureg.h> // ureg_t
#include <syscall_int.h> // GETTID_INT
#include <ctrl_blk.h> // thread_lists
#include <console.h> // install_console
#include <timer.h> // install_timer
#include <system_call.h> // handle_gettid
#include <fault_handler.h> // handle_page_fault
#include <keyboard.h> // install_keyboard
#include <stdbool.h> // true
#include <simics.h> // lprintf
#include <virtual_interrupt.h> // initialize_virtual_interrupt
#include <keyhelp.h> // KEY_IDT_ENTRY
#include <timer_defines.h> // TIMER_IDT_ENTRY

// Put into IDT a dummy gate for the interrupt vector.
// The gate will be a trap gate with DPL 0 and the corresponding
// handler wrapper (namely wrap_handler_<interrupt>).
#define ADD_DUMMY_GATE(interrupt) \
    ADD_DUMMY_GATE_FOR_INTERRUPT(interrupt)
#define ADD_DUMMY_GATE_FOR_INTERRUPT(interrupt) \
    add_trap_gate(interrupt, wrap_handler##interrupt, KERNEL_PL)

typedef union gate_t {
    struct {
        uint16_t offset1;
        uint16_t segment_selector;
        uint16_t reserved_bits      : 5;
        uint16_t padding1           : 6;
        uint16_t d                  : 1;
        uint16_t padding2           : 1;
        uint16_t dpl                : 2;
        uint16_t p                  : 1;
        uint16_t offset2;
    } trap_gate;
    struct {
        uint16_t offset1;
        uint16_t segment_selector;
        uint16_t reserved_bits      : 5;
        uint16_t padding1           : 6;
        uint16_t d                  : 1;
        uint16_t padding2           : 1;
        uint16_t dpl                : 2;
        uint16_t p                  : 1;
        uint16_t offset2;
    } interrupt_gate;
} gate_t;


// The global handler array searched by the handle function.
// Mind the ureg_ptr argument passed to each handler! Changing
// what it points to may cause the handler to return to
// nowhere.
void (*handler_array[IDT_ENTS])(ureg_t *ureg_ptr) = {NULL};

/**
 * @brief initialize interrupt handlers and fill up IDT
 * 
 * @return a negative value on failure, 0 otherwise
 */
int initialize_idt(void) {
    ADD_DUMMY_GATE(0);
    ADD_DUMMY_GATE(1);
    ADD_DUMMY_GATE(2);
    ADD_DUMMY_GATE(3);
    ADD_DUMMY_GATE(4);
    ADD_DUMMY_GATE(5);
    ADD_DUMMY_GATE(6);
    ADD_DUMMY_GATE(7);
    ADD_DUMMY_GATE(8);
    ADD_DUMMY_GATE(9);
    ADD_DUMMY_GATE(10);
    ADD_DUMMY_GATE(11);
    ADD_DUMMY_GATE(12);
    ADD_DUMMY_GATE(13);
    ADD_DUMMY_GATE(14);
    ADD_DUMMY_GATE(15);
    ADD_DUMMY_GATE(16);
    ADD_DUMMY_GATE(17);
    ADD_DUMMY_GATE(18);
    ADD_DUMMY_GATE(19);
    ADD_DUMMY_GATE(20);
    ADD_DUMMY_GATE(21);
    ADD_DUMMY_GATE(22);
    ADD_DUMMY_GATE(23);
    ADD_DUMMY_GATE(24);
    ADD_DUMMY_GATE(25);
    ADD_DUMMY_GATE(26);
    ADD_DUMMY_GATE(27);
    ADD_DUMMY_GATE(28);
    ADD_DUMMY_GATE(29);
    ADD_DUMMY_GATE(30);
    ADD_DUMMY_GATE(31);
    ADD_DUMMY_GATE(32);
    ADD_DUMMY_GATE(33);
    ADD_DUMMY_GATE(34);
    ADD_DUMMY_GATE(35);
    ADD_DUMMY_GATE(36);
    ADD_DUMMY_GATE(37);
    ADD_DUMMY_GATE(38);
    ADD_DUMMY_GATE(39);
    ADD_DUMMY_GATE(40);
    ADD_DUMMY_GATE(41);
    ADD_DUMMY_GATE(42);
    ADD_DUMMY_GATE(43);
    ADD_DUMMY_GATE(44);
    ADD_DUMMY_GATE(45);
    ADD_DUMMY_GATE(46);
    ADD_DUMMY_GATE(47);
    ADD_DUMMY_GATE(48);
    ADD_DUMMY_GATE(49);
    ADD_DUMMY_GATE(50);
    ADD_DUMMY_GATE(51);
    ADD_DUMMY_GATE(52);
    ADD_DUMMY_GATE(53);
    ADD_DUMMY_GATE(54);
    ADD_DUMMY_GATE(55);
    ADD_DUMMY_GATE(56);
    ADD_DUMMY_GATE(57);
    ADD_DUMMY_GATE(58);
    ADD_DUMMY_GATE(59);
    ADD_DUMMY_GATE(60);
    ADD_DUMMY_GATE(61);
    ADD_DUMMY_GATE(62);
    ADD_DUMMY_GATE(63);
    ADD_DUMMY_GATE(64);
    ADD_DUMMY_GATE(65);
    ADD_DUMMY_GATE(66);
    ADD_DUMMY_GATE(67);
    ADD_DUMMY_GATE(68);
    ADD_DUMMY_GATE(69);
    ADD_DUMMY_GATE(70);
    ADD_DUMMY_GATE(71);
    ADD_DUMMY_GATE(72);
    ADD_DUMMY_GATE(73);
    ADD_DUMMY_GATE(74);
    ADD_DUMMY_GATE(75);
    ADD_DUMMY_GATE(76);
    ADD_DUMMY_GATE(77);
    ADD_DUMMY_GATE(78);
    ADD_DUMMY_GATE(79);
    ADD_DUMMY_GATE(80);
    ADD_DUMMY_GATE(81);
    ADD_DUMMY_GATE(82);
    ADD_DUMMY_GATE(83);
    ADD_DUMMY_GATE(84);
    ADD_DUMMY_GATE(85);
    ADD_DUMMY_GATE(86);
    ADD_DUMMY_GATE(87);
    ADD_DUMMY_GATE(88);
    ADD_DUMMY_GATE(89);
    ADD_DUMMY_GATE(90);
    ADD_DUMMY_GATE(91);
    ADD_DUMMY_GATE(92);
    ADD_DUMMY_GATE(93);
    ADD_DUMMY_GATE(94);
    ADD_DUMMY_GATE(95);
    ADD_DUMMY_GATE(96);
    ADD_DUMMY_GATE(97);
    ADD_DUMMY_GATE(98);
    ADD_DUMMY_GATE(99);
    ADD_DUMMY_GATE(100);
    ADD_DUMMY_GATE(101);
    ADD_DUMMY_GATE(102);
    ADD_DUMMY_GATE(103);
    ADD_DUMMY_GATE(104);
    ADD_DUMMY_GATE(105);
    ADD_DUMMY_GATE(106);
    ADD_DUMMY_GATE(107);
    ADD_DUMMY_GATE(108);
    ADD_DUMMY_GATE(109);
    ADD_DUMMY_GATE(110);
    ADD_DUMMY_GATE(111);
    ADD_DUMMY_GATE(112);
    ADD_DUMMY_GATE(113);
    ADD_DUMMY_GATE(114);
    ADD_DUMMY_GATE(115);
    ADD_DUMMY_GATE(116);
    ADD_DUMMY_GATE(117);
    ADD_DUMMY_GATE(118);
    ADD_DUMMY_GATE(119);
    ADD_DUMMY_GATE(120);
    ADD_DUMMY_GATE(121);
    ADD_DUMMY_GATE(122);
    ADD_DUMMY_GATE(123);
    ADD_DUMMY_GATE(124);
    ADD_DUMMY_GATE(125);
    ADD_DUMMY_GATE(126);
    ADD_DUMMY_GATE(127);
    ADD_DUMMY_GATE(128);
    ADD_DUMMY_GATE(129);
    ADD_DUMMY_GATE(130);
    ADD_DUMMY_GATE(131);
    ADD_DUMMY_GATE(132);
    ADD_DUMMY_GATE(133);
    ADD_DUMMY_GATE(134);
    ADD_DUMMY_GATE(135);
    ADD_DUMMY_GATE(136);
    ADD_DUMMY_GATE(137);
    ADD_DUMMY_GATE(138);
    ADD_DUMMY_GATE(139);
    ADD_DUMMY_GATE(140);
    ADD_DUMMY_GATE(141);
    ADD_DUMMY_GATE(142);
    ADD_DUMMY_GATE(143);
    ADD_DUMMY_GATE(144);
    ADD_DUMMY_GATE(145);
    ADD_DUMMY_GATE(146);
    ADD_DUMMY_GATE(147);
    ADD_DUMMY_GATE(148);
    ADD_DUMMY_GATE(149);
    ADD_DUMMY_GATE(150);
    ADD_DUMMY_GATE(151);
    ADD_DUMMY_GATE(152);
    ADD_DUMMY_GATE(153);
    ADD_DUMMY_GATE(154);
    ADD_DUMMY_GATE(155);
    ADD_DUMMY_GATE(156);
    ADD_DUMMY_GATE(157);
    ADD_DUMMY_GATE(158);
    ADD_DUMMY_GATE(159);
    ADD_DUMMY_GATE(160);
    ADD_DUMMY_GATE(161);
    ADD_DUMMY_GATE(162);
    ADD_DUMMY_GATE(163);
    ADD_DUMMY_GATE(164);
    ADD_DUMMY_GATE(165);
    ADD_DUMMY_GATE(166);
    ADD_DUMMY_GATE(167);
    ADD_DUMMY_GATE(168);
    ADD_DUMMY_GATE(169);
    ADD_DUMMY_GATE(170);
    ADD_DUMMY_GATE(171);
    ADD_DUMMY_GATE(172);
    ADD_DUMMY_GATE(173);
    ADD_DUMMY_GATE(174);
    ADD_DUMMY_GATE(175);
    ADD_DUMMY_GATE(176);
    ADD_DUMMY_GATE(177);
    ADD_DUMMY_GATE(178);
    ADD_DUMMY_GATE(179);
    ADD_DUMMY_GATE(180);
    ADD_DUMMY_GATE(181);
    ADD_DUMMY_GATE(182);
    ADD_DUMMY_GATE(183);
    ADD_DUMMY_GATE(184);
    ADD_DUMMY_GATE(185);
    ADD_DUMMY_GATE(186);
    ADD_DUMMY_GATE(187);
    ADD_DUMMY_GATE(188);
    ADD_DUMMY_GATE(189);
    ADD_DUMMY_GATE(190);
    ADD_DUMMY_GATE(191);
    ADD_DUMMY_GATE(192);
    ADD_DUMMY_GATE(193);
    ADD_DUMMY_GATE(194);
    ADD_DUMMY_GATE(195);
    ADD_DUMMY_GATE(196);
    ADD_DUMMY_GATE(197);
    ADD_DUMMY_GATE(198);
    ADD_DUMMY_GATE(199);
    ADD_DUMMY_GATE(200);
    ADD_DUMMY_GATE(201);
    ADD_DUMMY_GATE(202);
    ADD_DUMMY_GATE(203);
    ADD_DUMMY_GATE(204);
    ADD_DUMMY_GATE(205);
    ADD_DUMMY_GATE(206);
    ADD_DUMMY_GATE(207);
    ADD_DUMMY_GATE(208);
    ADD_DUMMY_GATE(209);
    ADD_DUMMY_GATE(210);
    ADD_DUMMY_GATE(211);
    ADD_DUMMY_GATE(212);
    ADD_DUMMY_GATE(213);
    ADD_DUMMY_GATE(214);
    ADD_DUMMY_GATE(215);
    ADD_DUMMY_GATE(216);
    ADD_DUMMY_GATE(217);
    ADD_DUMMY_GATE(218);
    ADD_DUMMY_GATE(219);
    ADD_DUMMY_GATE(220);
    ADD_DUMMY_GATE(221);
    ADD_DUMMY_GATE(222);
    ADD_DUMMY_GATE(223);
    ADD_DUMMY_GATE(224);
    ADD_DUMMY_GATE(225);
    ADD_DUMMY_GATE(226);
    ADD_DUMMY_GATE(227);
    ADD_DUMMY_GATE(228);
    ADD_DUMMY_GATE(229);
    ADD_DUMMY_GATE(230);
    ADD_DUMMY_GATE(231);
    ADD_DUMMY_GATE(232);
    ADD_DUMMY_GATE(233);
    ADD_DUMMY_GATE(234);
    ADD_DUMMY_GATE(235);
    ADD_DUMMY_GATE(236);
    ADD_DUMMY_GATE(237);
    ADD_DUMMY_GATE(238);
    ADD_DUMMY_GATE(239);
    ADD_DUMMY_GATE(240);
    ADD_DUMMY_GATE(241);
    ADD_DUMMY_GATE(242);
    ADD_DUMMY_GATE(243);
    ADD_DUMMY_GATE(244);
    ADD_DUMMY_GATE(245);
    ADD_DUMMY_GATE(246);
    ADD_DUMMY_GATE(247);
    ADD_DUMMY_GATE(248);
    ADD_DUMMY_GATE(249);
    ADD_DUMMY_GATE(250);
    ADD_DUMMY_GATE(251);
    ADD_DUMMY_GATE(252);
    ADD_DUMMY_GATE(253);
    ADD_DUMMY_GATE(254);
    ADD_DUMMY_GATE(255);

    // Fault handlers use trap gates with KERNEL_PL
    handler_array[IDT_PF] = handle_page_fault;
    handler_array[IDT_SS] = handle_seg_fault;
    handler_array[IDT_DE] = handle_div_zero_fault;

    // Device drivers use interrupt gates with KERNEL_PL.
    install_console();
    install_timer(NULL);
    install_keyboard();

    // System call handlers use trap gates with USER_PL
    if (initialize_system_call() < 0) {
        return -1;
    }
    handler_array[GETTID_INT] = handle_gettid;
    add_trap_gate(GETTID_INT, wrap_handler72, USER_PL);
    handler_array[FORK_INT] = handle_fork;
    add_trap_gate(FORK_INT, wrap_handler65, USER_PL);
    handler_array[EXEC_INT] = handle_exec;
    add_trap_gate(EXEC_INT, wrap_handler66, USER_PL);
    handler_array[HALT_INT] = handle_halt;
    add_trap_gate(HALT_INT, wrap_handler85, USER_PL);
    handler_array[WAIT_INT] = handle_wait;
    add_trap_gate(WAIT_INT, wrap_handler68, USER_PL);
    handler_array[VANISH_INT] = handle_vanish;
    add_trap_gate(VANISH_INT, wrap_handler96, USER_PL);
    handler_array[TASK_VANISH_INT] = handle_task_vanish;
    add_trap_gate(TASK_VANISH_INT, wrap_handler87, USER_PL);
    handler_array[SET_STATUS_INT] = handle_set_status;
    add_trap_gate(SET_STATUS_INT, wrap_handler89, USER_PL);
    handler_array[YIELD_INT] = handle_yield;
    add_trap_gate(YIELD_INT, wrap_handler69, USER_PL);
    handler_array[DESCHEDULE_INT] = handle_deschedule;
    add_trap_gate(DESCHEDULE_INT, wrap_handler70, USER_PL);
    handler_array[MAKE_RUNNABLE_INT] = handle_make_runnable;
    add_trap_gate(MAKE_RUNNABLE_INT, wrap_handler71, USER_PL);
    handler_array[PRINT_INT] = handle_print;
    add_trap_gate(PRINT_INT, wrap_handler78, USER_PL);
    handler_array[READLINE_INT] = handle_readline;
    add_trap_gate(READLINE_INT, wrap_handler77, USER_PL);
    handler_array[GETCHAR_INT] = handle_getchar;
    add_trap_gate(GETCHAR_INT, wrap_handler76, USER_PL);
    handler_array[NEW_PAGES_INT] = handle_new_pages;
    add_trap_gate(NEW_PAGES_INT, wrap_handler73, USER_PL);
    handler_array[REMOVE_PAGES_INT] = handle_remove_pages;
    add_trap_gate(REMOVE_PAGES_INT, wrap_handler74, USER_PL);
    handler_array[SWEXN_INT] = handle_swexn;
    add_trap_gate(SWEXN_INT, wrap_handler116, USER_PL);
    handler_array[SLEEP_INT] = handle_sleep;
    add_trap_gate(SLEEP_INT, wrap_handler75, USER_PL);
    handler_array[READFILE_INT] = handle_readfile;
    add_trap_gate(READFILE_INT, wrap_handler98, USER_PL);
    handler_array[THREAD_FORK_INT] = handle_thread_fork;
    add_trap_gate(THREAD_FORK_INT, wrap_handler82, USER_PL);
    handler_array[GET_TICKS_INT] = handle_get_ticks;
    add_trap_gate(GET_TICKS_INT, wrap_handler83, USER_PL);
    handler_array[SET_TERM_COLOR_INT] = handle_set_term_color;
    add_trap_gate(SET_TERM_COLOR_INT, wrap_handler79, USER_PL);
    handler_array[SET_CURSOR_POS_INT] = handle_set_cursor_pos;
    add_trap_gate(SET_CURSOR_POS_INT, wrap_handler80, USER_PL);
    handler_array[GET_CURSOR_POS_INT] = handle_get_cursor_pos;
    add_trap_gate(GET_CURSOR_POS_INT, wrap_handler81, USER_PL);
    handler_array[NEW_CONSOLE_INT] = handle_new_console;
    add_trap_gate(NEW_CONSOLE_INT, wrap_handler88, USER_PL);

    // hypervisor specific
    initialize_virtual_interrupt();

    lprintf("IDT is initialized.");
    return 0;
}

/**
 * @brief Every handler wrapper will call this function, which
 *        will find the right handler to run.
 * 
 * @param ureg_ptr the execution state before interrupt happens
 */
void handle(ureg_t *ureg_ptr) {
    unsigned int interrupt = ureg_ptr->cause;
    tcb_t *current_tcb_ptr = thread_lists[RUNNING_STATE]->data;
    pcb_t *current_pcb_ptr = current_tcb_ptr->pcb_ptr;
    if (current_pcb_ptr->guest) {
        if (ureg_ptr->cs == SEGSEL_KERNEL_CS) {
            if (interrupt != TIMER_IDT_ENTRY && interrupt != KEY_IDT_ENTRY) {
                crash_guest();
                return;
            }
        } else {
            handle_virtual_interrupt(ureg_ptr);
            return;
        }
    }

    // if interrupt is one of faults we need to handle,
    // and PCB has user provided handler
    //     consume user provided handler
    //     switch to user provided exception stack, copy ureg,
    //     change %ds, %es, %fs, %gs,
    //     run user provided handler and restore registers
    // else
    //     (interrupt is system call or hardware interrupt or fault)
    //     search handler array for handler
    //     if handler is found
    //         run handler
    //     else
    //         lprintf("Interrupt %d is not handled.", interrupt)
    
    if (
        (interrupt == IDT_DE || interrupt == IDT_NP || interrupt == IDT_PF) &&
        (current_tcb_ptr->exception_stack != NULL)
    ) {
        void *exception_stack = current_tcb_ptr->exception_stack;
        void (*handler)(void *arg, ureg_t *ureg_ptr) =
            current_tcb_ptr->handler;
        void *arg = current_tcb_ptr->arg;
        current_tcb_ptr->exception_stack = NULL;
        wrap_user_provided_handler(
            ureg_ptr,
            exception_stack,
            handler,
            arg
        );
        return;
    }
    if (handler_array[interrupt] != NULL) {
        handler_array[interrupt](ureg_ptr);
        return;
    }
    lprintf("Interrupt %u is not handled.", interrupt);
    fault_kill_thread();
}

/**
 * @brief add a trap gate to IDT
 * 
 * Interrupts are enabled during the execution of handling routines
 * registered in trap gates.
 * 
 * @param interrupt the interrupt vector 
 * @param handler_wrapper pointer to the function to handle this interrupt
 * @param dpl DPL of the gate
 */
void add_trap_gate(
    int interrupt,
    void (*handler_wrapper)(void),
    uint32_t dpl
) {
    gate_t *idt_ptr = (gate_t *) (idt_base() + interrupt * sizeof(gate_t));
    idt_ptr->trap_gate.p = 1;
    idt_ptr->trap_gate.dpl = dpl;
    idt_ptr->trap_gate.padding1 = 0b111000;
    idt_ptr->trap_gate.padding2 = 0;
    idt_ptr->trap_gate.d = 1;
    idt_ptr->trap_gate.offset1 = (uint32_t) handler_wrapper;
    idt_ptr->trap_gate.offset2 = ((uint32_t) handler_wrapper) >>
        (sizeof(uint16_t) * CHAR_BIT);
    idt_ptr->trap_gate.segment_selector = SEGSEL_KERNEL_CS;
}

/**
 * @brief add an interrupt gate to IDT
 * 
 * Interrupts are disabled during the execution of handling routines
 * registered in trap gates.
 * 
 * @param interrupt the interrupt vector 
 * @param handler_wrapper pointer to the function to handle this interrupt
 * @param dpl DPL of the gate
 */
void add_interrupt_gate(
    int interrupt,
    void (*handler_wrapper)(void),
    uint32_t dpl
) {
    gate_t *idt_ptr = (gate_t *) (idt_base() + interrupt * sizeof(gate_t));
    idt_ptr->interrupt_gate.p = 1;
    idt_ptr->interrupt_gate.dpl = dpl;
    idt_ptr->interrupt_gate.padding1 = 0b110000;
    idt_ptr->interrupt_gate.padding2 = 0;
    idt_ptr->interrupt_gate.d = 1;
    idt_ptr->trap_gate.offset1 = (uint32_t) handler_wrapper;
    idt_ptr->trap_gate.offset2 = ((uint32_t) handler_wrapper) >>
        (sizeof(uint16_t) * CHAR_BIT);
    idt_ptr->interrupt_gate.segment_selector = SEGSEL_KERNEL_CS;
}
