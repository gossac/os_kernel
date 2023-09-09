#include <syscall_int.h>

.global get_ticks /* unsigned int get_ticks(void); */

get_ticks:
	push %ebp
	mov %esp, %ebp

	int $GET_TICKS_INT
	
	mov %ebp, %esp
	pop %ebp
    ret
