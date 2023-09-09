#include <syscall_int.h>

.global set_status /* void set_status(int status); */

set_status:
	push %ebp
	mov %esp, %ebp
	push %esi

    mov 8(%ebp), %esi
	int $SET_STATUS_INT
	
	mov -4(%ebp), %esi
	mov %ebp, %esp
	pop %ebp
    ret
