#include <syscall_int.h>

.global halt /* void halt(void); */

halt:
    push %ebp
    mov %esp, %ebp

    int $HALT_INT

    mov %ebp, %esp
    pop %ebp
    ret
