#include <syscall_int.h>

.global remove_pages /* int remove_pages(void * addr); */

remove_pages:
    push %ebp
    mov %esp, %ebp
    push %esi

    mov 8(%ebp), %esi
    int $REMOVE_PAGES_INT

    mov -4(%ebp), %esi
    mov %ebp, %esp
    pop %ebp
    ret
