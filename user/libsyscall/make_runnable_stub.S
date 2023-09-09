#include <syscall_int.h>

.global make_runnable /* int make_runnable(int pid); */

make_runnable:
	push %ebp
	mov %esp, %ebp
	push %esi

    mov 8(%ebp), %esi
	int $MAKE_RUNNABLE_INT
	
	mov -4(%ebp), %esi
	mov %ebp, %esp
	pop %ebp
    ret
