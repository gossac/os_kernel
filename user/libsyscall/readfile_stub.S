#include <syscall_int.h>

.global readfile /* int readfile(char *filename, char *buf, int count, int offset); */

readfile:
    push %ebp
    mov %esp, %ebp
    push %esi

    lea 8(%ebp), %esi
    int $READFILE_INT

    mov -4(%ebp), %esi
    mov %ebp, %esp
    pop %ebp
    ret
