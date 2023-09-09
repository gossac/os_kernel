#include <syscall_int.h>

.global yield /* int yield(int pid); */

yield:
	push %ebp
	mov %esp, %ebp
	push %esi

    mov 8(%ebp), %esi
	int $YIELD_INT
	
	mov -4(%ebp), %esi
	mov %ebp, %esp
	pop %ebp
    ret
