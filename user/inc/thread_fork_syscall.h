#ifndef __THREAD_FORK_SYSCALL_H__
#define __THREAD_FORK_SYSCALL_H__

/**
 * @brief the stub for the thread_fork system call
 * 
 * @param stack the address four bytes higher than the first slot of the new thread's stack
 * @param wrapper a wrapper function which will call the new thread's enclosing function
 */
int thread_fork(void *stack, void (*wrapper)(void));

#endif /* __THREAD_FORK_SYSCALL_H__ */