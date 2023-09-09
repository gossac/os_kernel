/** @file kernel.c
 *  @brief kernel startup
 *
 *  @author Tony Xi (xiaolix)
 *  @author Zekun Ma (zekunm)
 */

#include <common_kern.h>

/* libc includes. */
#include <stdio.h>
#include <simics.h>                 /* lprintf() */

/* multiboot header file */
#include <multiboot.h>              /* boot_info */

/* x86 specific includes */
#include <x86/asm.h>                /* enable_interrupts() */

#include <interrupt.h>
#include <ctrl_blk.h>
#include <vm.h>
#include <console.h>
#include <execution_state.h>
#include <loader.h>
#include <cr.h>
#include <stdint.h>
#include <ureg.h>
#include <assert.h>
#include <mem_allocation.h>
#include <segmentation.h>

volatile static int __kernel_all_done = 0;

/** @brief Kernel entrypoint.
 *  
 *  This is the entrypoint for the kernel.
 *
 * @return Does not return
 */
int kernel_main(mbinfo_t *mbinfo, int argc, char **argv, char **envp)
{
    // placate compiler
    (void)mbinfo;
    (void)argc;
    (void)argv;
    (void)envp;

    lprintf("Hello from a brand new kernel!");

    affirm(!hv_isguest());

    // Initialize GDT.
    initialize_gdt();

    // Initialize malloc wrappers.
    affirm(!(initialize_mem_allocation() < 0));

    // Initialize IDT.
    affirm(!(initialize_idt() < 0));

    // Clear console.
    clear_console();

    // Initialize physical frame allocator and page directory manager.
    affirm(!(init_page_dir_manager() < 0));

    // Set up the first TCB.
    affirm(!(init_ctrl_blk() < 0));

    // Initialize esp0.
    tcb_t *current_tcb_ptr = thread_lists[RUNNING_STATE]->data;
    set_esp0((uint32_t)(current_tcb_ptr->kernel_stack + KERNEL_STACK_LEN));
    lprintf("esp0 is initialized.");

    // Initialize control registers.
    set_cr3(
        (get_cr3() & ~((~0 >> PAGE_SHIFT) << PAGE_SHIFT)) |
        (uint32_t)current_tcb_ptr->pcb_ptr->page_directory
    );
    set_cr0(get_cr0() | CR0_PG);
    set_cr4(get_cr4() | CR4_PGE);
    lprintf("Control registers are initialized.");

    // Load init.
    ureg_t ureg;
    affirm (!(
        load_executable(
            "init",
            (char *[]){"init", NULL},
            &ureg
        ) < 0 ||
        fork_ctrl_blk(&ureg) < 0
    ));

    // Load idle.
    affirm(!(load_executable("idle", (char *[]){"idle", NULL}, &ureg) < 0));

    // lprintf(
    //     "Some of the fields in ureg_t: "
    //     "ds = 0x%x, es = 0x%x, fs = 0x%x, gs = 0x%x, "
    //     "edi = 0x%x, esi = 0x%x, ebp = %p, zero = %p, "
    //     "ebx = 0x%x, edx = 0x%x, ecx = 0x%x, eax = 0x%x, "
    //     "ss = 0x%x, esp = %p, eflags = 0x%x, cs = 0x%x, eip = %p",
    //     ureg.ds, ureg.es, ureg.fs, ureg.gs,
    //     ureg.edi, ureg.esi, (void *)ureg.ebp, (void *)ureg.zero,
    //     ureg.ebx, ureg.edx, ureg.ecx, ureg.eax,
    //     ureg.ss, (void *)ureg.esp, ureg.eflags, ureg.cs, (void *)ureg.eip
    // );
    lprintf("Run!");
    load_ureg(&ureg);

    while (!__kernel_all_done) {
        continue;
    }

    return 0;
}
