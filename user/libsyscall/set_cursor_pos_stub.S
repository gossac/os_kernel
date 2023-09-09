#include <syscall_int.h>

.global set_cursor_pos /* int set_cursor_pos(int row, int col); */

set_cursor_pos:
    push %ebp
    mov %esp, %ebp
    push %esi

    lea 8(%ebp), %esi
    int $SET_CURSOR_POS_INT

    mov -4(%ebp), %esi
    mov %ebp, %esp
    pop %ebp
    ret
