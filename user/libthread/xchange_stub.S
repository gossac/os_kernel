
.global xchange	/* int xchange(int *lock, int val); */

xchange:
    pushl %ebp		    /* save registers and stack pointers */
    movl %esp, %ebp	
    pushl %edi
    pushl %esi

    movl 8(%ebp), %edi  /* move lock into edi */
    movl 12(%ebp), %esi /* move val into esi */
    xchg (%edi), %esi   /* atomically exchange values */
    movl %esi, %eax     /* return what was in the lock */

    movl -8(%ebp), %esi /* restore registers and stack pointers */
    movl -4(%ebp), %edi
    movl %ebp, %esp
    popl %ebp
    ret
