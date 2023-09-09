/**
 * @file context_switcher.c
 * @author Tony Xi (xiaolix)
 * @author Zekun Ma (zekunm)
 * @brief procedure of context switching
 */

#include <context_switcher.h> // switch_context
#include <ctrl_blk.h> // tcb_t
#include <context.h> // save_and_load
#include <stdint.h> // uint32_t
#include <stddef.h> // NULL
#include <simics.h> // lprintf

/**
 * @brief switch the current running thread, including changing the
 *        states of both the thread switching in and the thread
 *        switching out, saving the context of the former and loading
 *        the context of the latter
 * 
 * This function should be called only when interrupts are disabled.
 * 
 * @param target_tcb_ptr the thread to switch to
 * @param state the state that the original thread ends up with
 * @param blocking_detail_ptr pointer to the blocking reason, used only
 *                            when the original thread is turning into
 *                            WAITING_STATE.
 * @return A negative value on failure, in which case nothing will
 *         happen. 0 otherwise.
 */
int switch_context(tcb_t *target_tcb_ptr, int state, blocking_detail_t *blocking_detail_ptr) {
    if (target_tcb_ptr == NULL) {
        return -1;
    }
    if (!(state >= NEW_STATE && state <= TERMINATED_STATE)) {
        return -1;
    }
    if (state == WAITING_STATE && blocking_detail_ptr == NULL) {
        return -1;
    }

    // lprintf("--- before alter_state ---");
    // lprintf("threads in RUNNING_STATE:");
    // if (thread_lists[RUNNING_STATE] != NULL) {
    //     tcb_ptr_node_t *node_ptr = thread_lists[RUNNING_STATE];
    //     do {
    //         lprintf("thread %d", node_ptr->data->tid);
    //         node_ptr = node_ptr->next;
    //     } while (node_ptr != thread_lists[RUNNING_STATE]);
    // }
    // lprintf("threads in READY_STATE:");
    // if (thread_lists[READY_STATE] != NULL) {
    //     tcb_ptr_node_t *node_ptr = thread_lists[READY_STATE];
    //     do {
    //         lprintf("thread %d", node_ptr->data->tid);
    //         node_ptr = node_ptr->next;
    //     } while (node_ptr != thread_lists[READY_STATE]);
    // }
    tcb_t *original_tcb_ptr = thread_lists[RUNNING_STATE]->data;
    alter_state(original_tcb_ptr, state, blocking_detail_ptr);
    alter_state(target_tcb_ptr, RUNNING_STATE, NULL);
    // lprintf("--- after alter_state ---");
    // lprintf("threads in RUNNING_STATE:");
    // if (thread_lists[RUNNING_STATE] != NULL) {
    //     tcb_ptr_node_t *node_ptr = thread_lists[RUNNING_STATE];
    //     do {
    //         lprintf("thread %d", node_ptr->data->tid);
    //         node_ptr = node_ptr->next;
    //     } while (node_ptr != thread_lists[RUNNING_STATE]);
    // }
    // lprintf("threads in READY_STATE:");
    // if (thread_lists[READY_STATE] != NULL) {
    //     tcb_ptr_node_t *node_ptr = thread_lists[READY_STATE];
    //     do {
    //         lprintf("thread %d", node_ptr->data->tid);
    //         node_ptr = node_ptr->next;
    //     } while (node_ptr != thread_lists[READY_STATE]);
    // }

    set_esp0((uint32_t)(target_tcb_ptr->kernel_stack + KERNEL_STACK_LEN));
    // %cr3 will be reloaded in save_and_load.
    save_and_load(original_tcb_ptr, target_tcb_ptr);
    return 0;
}
