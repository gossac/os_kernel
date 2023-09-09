#include <syscall_int.h>

.global fork /* int fork(void); */

fork:
	push %ebp
	mov %esp, %ebp

	int $FORK_INT
	
	mov %ebp, %esp
	pop %ebp
    ret
