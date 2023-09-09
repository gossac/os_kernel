#include <syscall_int.h>

.global get_cursor_pos /* int get_cursor_pos(int *row, int *col); */

get_cursor_pos:
    push %ebp
    mov %esp, %ebp
    push %esi

    lea 8(%ebp), %esi
    int $GET_CURSOR_POS_INT

    mov -4(%ebp), %esi
    mov %ebp, %esp
    pop %ebp
    ret
