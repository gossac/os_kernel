#include <syscall_int.h>

.global swexn /* int swexn(void *esp3, swexn_handler_t eip, void *arg, ureg_t *newureg); */

swexn:
    push %ebp
    mov %esp, %ebp
    push %esi

    lea 8(%ebp), %esi
    int $SWEXN_INT

    mov -4(%ebp), %esi
    mov %ebp, %esp
    pop %ebp
    ret
