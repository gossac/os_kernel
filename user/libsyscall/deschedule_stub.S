#include <syscall_int.h>

.global deschedule /* int deschedule(int *flag); */

deschedule:
	push %ebp
	mov %esp, %ebp
	push %esi

    mov 8(%ebp), %esi
	int $DESCHEDULE_INT
	
	mov -4(%ebp), %esi
	mov %ebp, %esp
	pop %ebp
    ret
