#include <syscall_int.h>

.global sleep /* int sleep(int ticks); */

sleep:
	push %ebp
	mov %esp, %ebp
	push %esi

    mov 8(%ebp), %esi
	int $SLEEP_INT
	
	mov -4(%ebp), %esi
	mov %ebp, %esp
	pop %ebp
    ret
