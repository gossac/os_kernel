// void load_ureg(ureg_t *ureg_ptr);
.global load_ureg
load_ureg:
    pushl %ebp
    movl %esp, %ebp

    // ureg_ptr
    movl 8(%ebp), %eax

    // ureg_ptr->ss
    movl 76(%eax), %edx
    pushl %edx
    // ureg_ptr->esp
    movl 72(%eax), %edx
    pushl %edx
    // ureg_ptr->eflags
    movl 68(%eax), %edx
    pushl %edx
    // ureg_ptr->cs
    movl 64(%eax), %edx
    pushl %edx
    // ureg_ptr->eip
    movl 60(%eax), %edx
    pushl %edx
    // ureg_ptr->error_code
    movl 56(%eax), %edx
    pushl %edx
    // ureg_ptr->eax
    movl 52(%eax), %edx
    pushl %edx
    // ureg_ptr->ecx
    movl 48(%eax), %edx
    pushl %edx
    // ureg_ptr->edx
    movl 44(%eax), %edx
    pushl %edx
    // ureg_ptr->ebx
    movl 40(%eax), %edx
    pushl %edx
    // ureg_ptr->zero
    movl 36(%eax), %edx
    pushl %edx
    // ureg_ptr->ebp
    movl 32(%eax), %edx
    pushl %edx
    // ureg_ptr->esi
    movl 28(%eax), %edx
    pushl %edx
    // ureg_ptr->edi
    movl 24(%eax), %edx
    pushl %edx
    // ureg_ptr->gs
    movl 20(%eax), %edx
    pushl %edx
    // ureg_ptr->fs
    movl 16(%eax), %edx
    pushl %edx
    // ureg_ptr->es
    movl 12(%eax), %edx
    pushl %edx
    // ureg_ptr->ds
    movl 8(%eax), %edx
    pushl %edx
    // ureg_ptr->cr2
    movl 4(%eax), %edx
    pushl %edx
    // ureg_ptr->cause
    movl (%eax), %edx
    pushl %edx

    addl $8, %esp
    popl %ds
    popl %es
    popl %fs
    popl %gs
    popal
    addl $4, %esp
    iret
