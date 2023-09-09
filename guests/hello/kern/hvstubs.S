#include <hvcall_int.h>

.globl hv_magic
.globl hv_exit
.globl hv_setidt
.globl hv_disable_interrupts
.globl hv_enable_interrupts
.globl hv_setpd
.globl hv_adjustpg
.globl hv_print
.globl hv_cons_set_term_color
.globl hv_cons_set_cursor_pos
.globl hv_cons_get_cursor_pos
.globl hv_iret
.globl hv_print_at

hv_magic:
	pushl %ebp
	movl %esp, %ebp
	
	movl $HV_MAGIC_OP, %eax
	int $HV_INT

	movl %ebp, %esp
	popl %ebp
	ret

hv_exit:
	pushl %ebp
	movl %esp, %ebp

	movl 8(%ebp), %eax
	pushl %eax
	movl $HV_EXIT_OP, %eax
	int $HV_INT

	movl %ebp, %esp
	popl %ebp
	ret

hv_setidt:
	pushl %ebp
	movl %esp, %ebp

	movl 16(%ebp), %eax
	pushl %eax
	movl 12(%ebp), %eax
	pushl %eax
	movl 8(%ebp), %eax
	pushl %eax
	movl $HV_SETIDT_OP, %eax
	int $HV_INT

	movl %ebp, %esp
	popl %ebp
	ret

hv_disable_interrupts:
	pushl %ebp
	movl %esp, %ebp

	movl $HV_DISABLE_OP, %eax
	int $HV_INT

	movl %ebp, %esp
	popl %ebp
	ret

hv_enable_interrupts:
	pushl %ebp
	movl %esp, %ebp

	movl $HV_ENABLE_OP, %eax
	int $HV_INT

	movl %ebp, %esp
	popl %ebp
	ret

hv_setpd:
	pushl %ebp
	movl %esp, %ebp

	movl 12(%ebp), %eax
	pushl %eax
	movl 8(%ebp), %eax
	pushl %eax
	movl $HV_SETPD_OP, %eax
	int $HV_INT

	movl %ebp, %esp
	popl %ebp
	ret

hv_adjustpg:
	pushl %ebp
	movl %esp, %ebp

	movl 8(%ebp), %eax
	pushl %eax
	movl $HV_ADJUSTPG_OP, %eax
	int $HV_INT

	movl %ebp, %esp
	popl %ebp
	ret

hv_print:
	pushl %ebp
	movl %esp, %ebp

	movl 12(%ebp), %eax
	pushl %eax
	movl 8(%ebp), %eax
	pushl %eax
	movl $HV_PRINT_OP, %eax
	int $HV_INT

	movl %ebp, %esp
	popl %ebp
	ret

hv_cons_set_term_color:
	pushl %ebp
	movl %esp, %ebp

	movl 8(%ebp), %eax
	pushl %eax
	movl $HV_SET_COLOR_OP, %eax
	int $HV_INT

	movl %ebp, %esp
	popl %ebp
	ret

hv_cons_set_cursor_pos:
	pushl %ebp
	movl %esp, %ebp

	movl 12(%ebp), %eax
	pushl %eax
	movl 8(%ebp), %eax
	pushl %eax
	movl $HV_SET_CURSOR_OP, %eax
	int $HV_INT

	movl %ebp, %esp
	popl %ebp
	ret

hv_cons_get_cursor_pos:
	pushl %ebp
	movl %esp, %ebp

	movl 12(%ebp), %eax
	pushl %eax
	movl 8(%ebp), %eax
	pushl %eax
	movl $HV_GET_CURSOR_OP, %eax
	int $HV_INT

	movl %ebp, %esp
	popl %ebp
	ret

hv_iret:
	pushl %ebp
	movl %esp, %ebp

	movl 24(%ebp), %eax
	pushl %eax
	movl 20(%ebp), %eax
	pushl %eax
	movl 16(%ebp), %eax
	pushl %eax
	movl 12(%ebp), %eax
	pushl %eax
	movl 8(%ebp), %eax
	pushl %eax
	movl $HV_IRET_OP, %eax
	int $HV_INT

	movl %ebp, %esp
	popl %ebp
	ret

hv_print_at:
	pushl %ebp
	movl %esp, %ebp

	movl 24(%ebp), %eax
	pushl %eax
	movl 20(%ebp), %eax
	pushl %eax
	movl 16(%ebp), %eax
	pushl %eax
	movl 12(%ebp), %eax
	pushl %eax
	movl 8(%ebp), %eax
	pushl %eax
	movl $HV_PRINT_AT_OP, %eax
	int $HV_INT

	movl %ebp, %esp
	popl %ebp
	ret
