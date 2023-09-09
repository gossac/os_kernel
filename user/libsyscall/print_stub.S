#include <syscall_int.h>

.global print /* int print(int size, char *buf); */

print:
    push %ebp
    mov %esp, %ebp
    push %esi

    lea 8(%ebp), %esi
    int $PRINT_INT

    mov -4(%ebp), %esi
    mov %ebp, %esp
    pop %ebp
    ret
