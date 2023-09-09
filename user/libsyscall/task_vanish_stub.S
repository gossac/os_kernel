#include <syscall_int.h>

.global task_vanish	/* void task_vanish(int status) NORETURN; */

task_vanish:
	push %ebp
	mov %esp, %ebp
	push %esi

    mov 8(%ebp), %esi
	int $TASK_VANISH_INT
	
	mov -4(%ebp), %esi
	mov %ebp, %esp
	pop %ebp
    ret
