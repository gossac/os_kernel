#include <syscall_int.h>

.global gettid	/* int gettid(void); */

gettid:
	push %ebp
	mov %esp, %ebp

	int $GETTID_INT
	
	mov %ebp, %esp
	pop %ebp
    ret
