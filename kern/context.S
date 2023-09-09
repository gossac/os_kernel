// void save_ureg(tcb_t *new_tcb_ptr, ureg_t *ureg_ptr);
.global save_ureg
save_ureg:
    pushl %ebp
    movl %esp, %ebp

    // save callee saved registers
    pushl %ebx

    // new_tcb_ptr
    movl 8(%ebp), %eax
    // ureg_ptr
    movl 12(%ebp), %edx

    // save the current stack pointer and
    // switch to the new thread's kernel stack,
    // which is new_tcb_ptr->kernel_stack
    // 16400 = sizeof(new_tcb_ptr->pcb_ptr) +
    //         sizeof(new_tcb_ptr->tid) +
    //         sizeof(new_tcb_ptr->state) +
    //         sizeof(new_tcb_ptr->esp) +
    //         sizeof(new_tcb_ptr->kernel_stack)
    movl %esp, %ecx
    leal 16400(%eax), %esp
    
    // ureg_ptr->ss
    movl 76(%edx), %ebx
    pushl %ebx
    // ureg_ptr->esp
    movl 72(%edx), %ebx
    pushl %ebx
    // ureg_ptr->eflags
    movl 68(%edx), %ebx
    pushl %ebx
    // ureg_ptr->cs
    movl 64(%edx), %ebx
    pushl %ebx
    // ureg_ptr->eip
    movl 60(%edx), %ebx
    pushl %ebx
    // ureg_ptr->error_code
    movl 56(%edx), %ebx
    pushl %ebx
    // ureg_ptr->eax
    // For the new thread, fork and thread_fork should return 0. 
    pushl $0
    // ureg_ptr->ecx
    movl 48(%edx), %ebx
    pushl %ebx
    // ureg_ptr->edx
    movl 44(%edx), %ebx
    pushl %ebx
    // ureg_ptr->ebx
    movl 40(%edx), %ebx
    pushl %ebx
    // ureg_ptr->zero
    movl 36(%edx), %ebx
    pushl %ebx
    // ureg_ptr->ebp
    movl 32(%edx), %ebx
    pushl %ebx
    // ureg_ptr->esi
    movl 28(%edx), %ebx
    pushl %ebx
    // ureg_ptr->edi
    movl 24(%edx), %ebx
    pushl %ebx
    // ureg_ptr->gs
    movl 20(%edx), %ebx
    pushl %ebx
    // ureg_ptr->fs
    movl 16(%edx), %ebx
    pushl %ebx
    // ureg_ptr->es
    movl 12(%edx), %ebx
    pushl %ebx
    // ureg_ptr->ds
    movl 8(%edx), %ebx
    pushl %ebx
    // ureg_ptr->cr2
    movl 4(%edx), %ebx
    pushl %ebx
    // ureg_ptr->cause
    movl (%edx), %ebx
    pushl %ebx

    // We decided not to touch segment registers during context switch.
    // 4294966784 = ~0x200
    pushl $way_to_user_mode
    pushfl
    popl %ebx
    andl $4294966784, %ebx
    pushl %ebx
    pushal

    movl %esp, 12(%eax)

    // switch the kernel stack back
    movl %ecx, %esp

    // restore callee saved registers
    movl -4(%ebp), %ebx

    movl %ebp, %esp
    popl %ebp
    ret
way_to_user_mode:
    addl $8, %esp
    popl %ds
    popl %es
    popl %fs
    popl %gs
    popal
    addl $4, %esp
    iret

// void save_and_load(tcb_t *original_tcb_ptr, tcb_t *target_tcb_ptr);
.global save_and_load
save_and_load:
    // original_tcb_ptr
    movl 4(%esp), %eax
    // target_tcb_ptr
    movl 8(%esp), %edx

    pushfl
    pushal

    movl %esp, 12(%eax)

    // target_tcb_ptr->pcb_ptr
    movl (%edx), %eax
    // target_tcb_ptr->pcb_ptr->page_directory
    movl 16(%eax), %eax
    movl %cr3, %ecx
    // 4095 = 0xfff
    andl $4095, %ecx
    orl %ecx, %eax
    // set_cr3(
    //     (~((~0 >> PAGE_SHIFT) << PAGE_SHIFT) & get_cr3()) |
    //     target_tcb_ptr->pcb_ptr->page_directory
    // )
    movl %eax, %cr3
    
    movl 12(%edx), %esp

    popal
    popfl

    ret
