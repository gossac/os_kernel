#include <syscall_int.h>

.global thread_fork /* int thread_fork(void *stack, void (*wrapper)(void)); */

thread_fork:
    pushl %ebp
    movl %esp, %ebp
    pushl %ebx
    pushl %esi

    movl 8(%ebp), %ebx      /* ebx: stack */
    movl 12(%ebp), %esi     /* esi: wrapper */

    int $THREAD_FORK_INT    /* part ways */

    testl %eax, %eax        /* zero for created thread, nonzero for invoking thread */
    jnz way_for_caller      /* the invoking thread should return */

    movl %ebx, %esp         /* switch to the new stack */
    call *%esi              /* call wrapper which should never return */
way_for_caller:
    movl -8(%ebp), %esi
    movl -4(%ebp), %ebx
    movl %ebp, %esp     
    popl %ebp
    ret
