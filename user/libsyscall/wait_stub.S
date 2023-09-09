#include <syscall_int.h>

.global wait /* int wait(int *status_ptr); */

wait:
	push %ebp
	mov %esp, %ebp
	push %esi

    mov 8(%ebp), %esi
	int $WAIT_INT
	
	mov -4(%ebp), %esi
	mov %ebp, %esp
	pop %ebp
    ret
