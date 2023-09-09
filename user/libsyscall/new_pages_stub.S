#include <syscall_int.h>

.global new_pages /* int new_pages(void * addr, int len); */

new_pages:
    push %ebp
    mov %esp, %ebp
    push %esi

    lea 8(%ebp), %esi
    int $NEW_PAGES_INT

    mov -4(%ebp), %esi
    mov %ebp, %esp
    pop %ebp
    ret
