#include <syscall_int.h>

.global vanish /* void vanish(void); */

vanish:
	push %ebp
	mov %esp, %ebp

	int $VANISH_INT
	
	mov %ebp, %esp
	pop %ebp
    ret
