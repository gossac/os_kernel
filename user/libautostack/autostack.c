/**
 * @file autostack.c
 * @brief install a page fault exception handler that
 * automatically grows the stack for single threaded programs
 * 
 * @author Tony Xi (xiaolix)
 * @author Zekun Ma (zekunm)
 * @bug no known bug
 */

#include <syscall.h> // swexn
#include <stddef.h> // NULL
#include <ureg.h> // SWEXN_CAUSE_PAGEFAULT
#include <assert.h> // assert

#define AUTOSTACK_EXCEPTION_STACK_SIZE (2048) // the size of the exception stack by the byte
#define ROUND_STACK(stack) ((void *)((unsigned)(stack) / sizeof(int) * sizeof(int))) // round stack address to four bytes above the first slot

// This array is used as the exception stack.
char autostack_execption_stack_space[AUTOSTACK_EXCEPTION_STACK_SIZE] = {0};
void *current_stack_low = NULL;

void install_autostack(void * stack_high, void * stack_low);
/**
 * @brief handler of page fault exception due to automatic stack growth
 * 
 * @param arg the argument specified when the handler is set up, unused
 * @param ureg the execution state at the time of the exception
 */
void handle_autostack(void *arg, ureg_t *ureg);

void install_autostack(void *stack_high, void *stack_low) {
    // register the exception handler
    int handler_installation = swexn(
        ROUND_STACK(autostack_execption_stack_space + sizeof(autostack_execption_stack_space)),
        handle_autostack,
        NULL,
        NULL
    );
    (void)handler_installation;
    assert(handler_installation >= 0);
    current_stack_low = stack_low;
}

void handle_autostack(void *arg, ureg_t *ureg) {
    // detect the reason of the exception
    void *base = (void *)((ureg->cr2 / PAGE_SIZE) * PAGE_SIZE);
    if (
        ureg->cause != SWEXN_CAUSE_PAGEFAULT ||
        (ureg->error_code & 0x1) != 0 ||
        (unsigned)base / PAGE_SIZE * PAGE_SIZE != (unsigned)current_stack_low - PAGE_SIZE
    ) {
        swexn(NULL, NULL, NULL, ureg);
    }

    // allocate a new page
    int page_allocation = new_pages(base, PAGE_SIZE);
    if (page_allocation < 0) {
        swexn(NULL, NULL, NULL, ureg);
    }
    current_stack_low = (void *)((unsigned)current_stack_low - PAGE_SIZE);

    // register itself for the next exception and try the same instruction again
    swexn(
        ROUND_STACK(autostack_execption_stack_space + sizeof(autostack_execption_stack_space)),
        handle_autostack,
        NULL,
        ureg
    );

    swexn(NULL, NULL, NULL, ureg);
}
