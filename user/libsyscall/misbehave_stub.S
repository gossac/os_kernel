#include <syscall_int.h>

.global misbehave /* void misbehave(int mode); */

misbehave:
    push %ebp
    mov %esp, %ebp
    push %esi

    mov 8(%ebp), %esi
    int $MISBEHAVE_INT

    mov -4(%ebp), %esi
    mov %ebp, %esp
    pop %ebp
    ret
