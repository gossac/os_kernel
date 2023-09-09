.global xchange	/* int xchange(int *lock, int val); */

xchange:
    pushl %ebp          /* save frame pointer */
    movl %esp, %ebp	
    pushl %edi          /* save callee saved registers */
    pushl %esi

    movl 8(%ebp), %edi  /* move lock into edi */
    movl 12(%ebp), %esi /* move val into esi */
    xchgl (%edi), %esi  /* atomically exchange values */
    movl %esi, %eax     /* return what was in the lock */

    movl -8(%ebp), %esi /* restore callee saved registers */
    movl -4(%ebp), %edi
    movl %ebp, %esp     /* restore frame pointer */
    popl %ebp
    ret
