/**
 * @file thread.c
 * @brief thread management APIs and the thread crash handler
 * 
 * @author Tony Xi (xiaolix)
 * @author Zekun Ma (zekun)
 * @bug no known bug
 */

#include <thread.h> // thr_init
#include <stdbool.h> // bool
#include <mutex.h> // mutex_t
#include <stddef.h> // NULL
#include <linked_list.h> // list_node
#include <cond.h> // cond_t
#include <syscall.h> // swexn
#include <malloc.h> // malloc
#include <thread_fork_syscall.h> // thread_fork
#include <assert.h> // assert
#include <ureg.h> // ureg_t

// the size of the exception stack by the byte
#define THREAD_CRASH_EXCEPTION_STACK_SIZE (512)
// round stack address to four bytes above the first slot
#define ROUND_STACK(stack) \
    ((void *)((unsigned)(stack) / sizeof(int) * sizeof(int)))

/**
 * @brief Once a thread is created, there should be a
 * thread_info struct created as well to keep track of it.
 */
typedef struct thread_info {
    // the thread ID, same as the one returned by thread_fork
    int tid;

    // the stack space of the thread
    void *stack_space;
    // space used as the threads's exception stack
    char thread_crash_exception_stack_space[THREAD_CRASH_EXCEPTION_STACK_SIZE];
    // the thread's enclosing function
    void *(*func)(void *arg);
    // the argument fed to func
    void *arg;

    // whether another thread is joining on this thread
    bool is_joined_on;
    // a conditinal variable which the joining thread may wait on
    cond_t status_ready;
    // whether the thread has exited
    bool has_exited;
    // the exit status of the thread, usable only after has_exited turns true
    void *status;
} thread_info;

// mutex used by malloc family
mutex_t alloc_mutex;
// mutex used when changes happen over thread_info_chain
mutex_t thread_info_chain_mutex;
// the size of the thread's stack by the byte
int stack_size = 0;
/*
A linked list chaining information of every thread.

At any given point, the list can be like this,
where two reader threads represented by the
heading two nodes are holding reader locks, and
one writer thread following them is waiting. There's
also one reader thread coming after the writer
thread. So it has to be queued afterwards.
+---+     +---+     +---+     +---+
|   +---->|   +---->|   +---->|   |
| r |     | r |     | w |     | r |
|   |<----+   |<----+   |<----+   |
+---+     +---+     +---+     +---+ 

The list can also be like this, where a writer thread
represented by the first node is holding a writer lock.
And therefore all the writer threads and reader threads
queued after it have to wait.
+---+     +---+     +---+     +---+
|   +---->|   +---->|   +---->|   |
| w |     | w |     | r |     | r |
|   |<----+   |<----+   |<----+   |
+---+     +---+     +---+     +---+ 
*/
list_node *thread_info_chain = NULL;

// helper functions for internal use
/**
 * @brief the thread crash exception handler
 * 
 * @param arg the argument specified when the handler is set up, unused
 * @param ureg the execution state at the time of the exception
 */
void handle_thread_crash(void *arg, ureg_t *ureg);
/**
 * @brief the wrap function of a thread's enclosing function.
 *
 * This function should be passed to the @code{thread_fork} system call.
 * It does these three things:
 * 1. Set up the exception handler for the created thread.
 * 2. Call the enclosing function.
 * 3. Bring back the return value of the enclosing function.
 */
void wrap_func(void);
/**
 * @brief free the whole @code{thread_info_chain} list
 *        and all the resourecs related
 * 
 * For each thread recorded as a node in @code{thread_info_chain},
 * this function will destruct:
 * 1. Its stack space.
 * 2. The condition variable used by threads joining on it.
 * 3. The list node.
 * 
 * Please note that there are other resouces involved in the
 * thread library that are not in the charge of this function.
 */
void destruct_each_thread(void);

int thr_init(unsigned int size) {
    // give initial values to some global variables
    mutex_init(&alloc_mutex);
    mutex_init(&thread_info_chain_mutex);
    stack_size = size;
    
    // record the root thread itself into thread_info_chain
    thread_info *root_thread_info_ptr = malloc(sizeof(thread_info));
    if (root_thread_info_ptr == NULL) {
        return -1;
    }
    if (cond_init(&(root_thread_info_ptr->status_ready)) != 0) {
        free(root_thread_info_ptr);
        return -1;
    }
    root_thread_info_ptr->tid = gettid();
    root_thread_info_ptr->is_joined_on = false;
    root_thread_info_ptr->has_exited = false;
    root_thread_info_ptr->stack_space = NULL;
    thread_info_chain = append_node(NULL, root_thread_info_ptr);

    // de-register the autostack handler and regiter the thread crash handler
    if (
        swexn(
            ROUND_STACK(
                root_thread_info_ptr->thread_crash_exception_stack_space +
                sizeof(root_thread_info_ptr->thread_crash_exception_stack_space)
            ),
            handle_thread_crash,
            NULL,
            NULL
        ) < 0
    ) {
        free(root_thread_info_ptr);
        return -1;
    }
    return 0;
}

int thr_create(void *(*func)(void *args), void *args) {
    // do argument checking
    if (func == NULL) {
        return -1;
    }

    // Can I prepare a thread_info struct for the new thread?
    thread_info *new_thread_info_ptr = malloc(sizeof(thread_info));
    if (new_thread_info_ptr == NULL) {
        return -1;
    }

    // Can I initialize the condition variable used by the thread?
    if (cond_init(&(new_thread_info_ptr->status_ready)) != 0) {
        free (new_thread_info_ptr);
        return -1;
    }

    // Can I allocate the stack space for the new thread?
    new_thread_info_ptr->stack_space = malloc(stack_size);
    if (new_thread_info_ptr->stack_space == NULL) {
        free(new_thread_info_ptr);
        return -1;
    }

    // Now no more error would happen.
    // Just fill out thread_info and put it into thread_info_chain
    int tid;
    mutex_lock(&thread_info_chain_mutex);
    thread_info_chain = append_node(thread_info_chain, new_thread_info_ptr);
    new_thread_info_ptr->func = func;
    new_thread_info_ptr->arg = args;
    // Only after the thread is run can we get its ID and
    // put it into thread_info. So both operations should
    // be done in a single thread_info_chain_mutex guarded area.
    new_thread_info_ptr->tid = thread_fork(
        ROUND_STACK(new_thread_info_ptr->stack_space + stack_size),
        wrap_func
    );
    new_thread_info_ptr->is_joined_on = false;
    new_thread_info_ptr->has_exited = false;
    tid = new_thread_info_ptr->tid;
    mutex_unlock(&thread_info_chain_mutex);
    
    // Return the ID of the new thread.
    return tid;
}

int thr_join(int tid, void **statusp) {
    mutex_lock(&thread_info_chain_mutex);
    
    // search for the record of the joined thread
    list_node *joined_thread_info_node_ptr = NULL;
    if (thread_info_chain != NULL) {
        list_node *node_ptr = thread_info_chain;
        do {
            if (((thread_info *)(node_ptr->data))->tid == tid) {
               joined_thread_info_node_ptr = node_ptr;
               break; 
            }
            // searh from the oldest thread to the newest
            node_ptr = node_ptr->next;
        } while (node_ptr != thread_info_chain);
    }
    if (joined_thread_info_node_ptr == NULL) {
        mutex_unlock(&thread_info_chain_mutex);
        // Cannot find a thread with the specifed tid.
        // Maybe it has exited and been joined.
        // Maybe it has never come into being.
        return -1;
    }

    thread_info *joined_thread_info_ptr = joined_thread_info_node_ptr->data;
    if (joined_thread_info_ptr->has_exited) {
        // The thread has exited but never been joined.
        
        if (statusp != NULL) {
            *statusp = joined_thread_info_ptr->status;
        }

        // do cleanup for the joined thread, i.e.,
        // destroy the condition variable, de-allocate the stack
        // as well as the thread_info record of that joined thread
        cond_destroy(&(joined_thread_info_ptr->status_ready));
        free(joined_thread_info_ptr->stack_space);
        thread_info_chain = remove_node(
            thread_info_chain,
            joined_thread_info_node_ptr
        );

        mutex_unlock(&thread_info_chain_mutex);
        return 0;
    }

    if (joined_thread_info_ptr->is_joined_on) {
        mutex_unlock(&thread_info_chain_mutex);
        // There's already another thread joining on it.
        return -1;
    }

    // Now the thread has to wait until the joined thread is about
    // to exit and triggers the status_ready condition variable.
    joined_thread_info_ptr->is_joined_on = true;
    while (!joined_thread_info_ptr->has_exited) {
        cond_wait(
            &(joined_thread_info_ptr->status_ready),
            &thread_info_chain_mutex
        );
    }
    if (statusp != NULL) {
        *statusp = joined_thread_info_ptr->status;
    }
    

    // do cleanup for the joined thread, i.e.,
    // destroy the condition variable, de-allocate the stack
    // as well as the thread_info record of that joined thread
    cond_destroy(&(joined_thread_info_ptr->status_ready));
    free(joined_thread_info_ptr->stack_space);
    thread_info_chain = remove_node(
        thread_info_chain,
        joined_thread_info_node_ptr
    );
    
    mutex_unlock(&thread_info_chain_mutex);
    return 0;
}

void thr_exit(void *status) {
    int tid = gettid();

    mutex_lock(&thread_info_chain_mutex);

    // search the thread's own thread_into record
    list_node *current_thread_info_node_ptr = NULL;
    int thread_alive_count = 0;
    if (thread_info_chain != NULL) {
        list_node *node_ptr = thread_info_chain;
        do {
            if (((thread_info *)(node_ptr->data))->tid == tid) {
                current_thread_info_node_ptr = node_ptr;
            }
            if (!((thread_info *)(node_ptr->data))->has_exited) {
                thread_alive_count++;
            }
            // search from the oldest thread to the newest
            node_ptr = node_ptr->next;
        } while (node_ptr != thread_info_chain);
    }

    if (thread_alive_count <= 1) {
        // The caller of this thr_exit function is the root thread.

        destruct_each_thread();
        mutex_unlock(&thread_info_chain_mutex);

        mutex_destroy(&thread_info_chain_mutex);
        mutex_destroy(&alloc_mutex);

        task_vanish((int)status);
    } else {
        // The caller is a child thread.

        thread_info *current_thread_info_ptr = current_thread_info_node_ptr->data;

        // save exit status
        current_thread_info_ptr->has_exited = true;
        current_thread_info_ptr->status = status;
        
        // If there is any thread joining on it,
        // trigger the status_ready condition variable.
        if (current_thread_info_ptr->is_joined_on) {
            // There should be only one thread joining on it.
            cond_signal(&(current_thread_info_ptr->status_ready));
        }

        mutex_unlock(&thread_info_chain_mutex);
        vanish();
    }
}

int thr_getid(void) {
    return gettid();
}

int thr_yield(int tid) {
    if (yield(tid) < 0) {
        return -1;
    }
    return 0;
}

void destruct_each_thread(void) {
    // free all the resources owned by threads
    while (thread_info_chain != NULL) {
        list_node *last_thread_info_node_ptr = thread_info_chain->previous;
        thread_info *last_thread_info_ptr =
            last_thread_info_node_ptr->data;
        cond_destroy(&(last_thread_info_ptr->status_ready));
        free(last_thread_info_ptr->stack_space);
        // free from the newest thread to the oldest
        last_thread_info_node_ptr = last_thread_info_node_ptr->previous;
        thread_info_chain = remove_node(
            thread_info_chain,
            last_thread_info_node_ptr
        );
    }
}

void handle_thread_crash(void *arg, ureg_t *ureg) {
    mutex_lock(&thread_info_chain_mutex);
    destruct_each_thread();
    mutex_unlock(&thread_info_chain_mutex);

    mutex_destroy(&thread_info_chain_mutex);
    mutex_destroy(&alloc_mutex);

    task_vanish(-1);
}

void wrap_func(void) {
    int tid = gettid();
    
    void *thread_crash_exception_stack = NULL;
    void *(*func)(void *arg) = NULL;
    void *arg = NULL;

    // search the thread's own thread_info record,
    // and extract thread_crash_exception_stack_space, func and arg 
    mutex_lock(&thread_info_chain_mutex);
    list_node *node_ptr = thread_info_chain;
    do {
        thread_info *thread_info_ptr = node_ptr->data;
        if  (thread_info_ptr->tid == tid) {
            thread_crash_exception_stack = ROUND_STACK(
                thread_info_ptr->thread_crash_exception_stack_space +
                sizeof(thread_info_ptr->thread_crash_exception_stack_space)
            );
            func = thread_info_ptr->func;
            arg = thread_info_ptr->arg;
            break;
        }
        node_ptr = node_ptr->next;
    } while (node_ptr != thread_info_chain);
    mutex_unlock(&thread_info_chain_mutex);

    // regiter the thread crash handler
    int handler_installation = swexn (
        thread_crash_exception_stack,
        handle_thread_crash,
        NULL,
        NULL
    );
    (void)handler_installation;
    assert(handler_installation >= 0);

    // execute the enclosing function
    void *status = func(arg);

    thr_exit(status);
}