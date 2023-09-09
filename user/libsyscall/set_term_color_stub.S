#include <syscall_int.h>

.global set_term_color /* int set_term_color(int color); */

set_term_color:
    push %ebp
    mov %esp, %ebp
    push %esi

    mov 8(%ebp), %esi
    int $SET_TERM_COLOR_INT

    mov -4(%ebp), %esi
    mov %ebp, %esp
    pop %ebp
    ret
