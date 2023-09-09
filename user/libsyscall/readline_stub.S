#include <syscall_int.h>

.global readline /* int readline(int size, char *buf); */

readline:
    push %ebp
    mov %esp, %ebp
    push %esi

    lea 8(%ebp), %esi
    int $READLINE_INT

    mov -4(%ebp), %esi
    mov %ebp, %esp
    pop %ebp
    ret
