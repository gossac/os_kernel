#include <syscall_int.h>

.global getchar /* int getchar(void); */

getchar:
    push %ebp
    mov %esp, %ebp
    push %esi

    int $GETCHAR_INT

    mov -4(%ebp), %esi
    mov %ebp, %esp
    pop %ebp
    ret
