#include <syscall_int.h>

.global exec /* int exec(char *execname, char *argvec[]); */

exec:
	push %ebp
	mov %esp, %ebp
	push %esi

    lea 8(%ebp), %esi
	int $EXEC_INT
	
	mov -4(%ebp), %esi
	mov %ebp, %esp
	pop %ebp
    ret
