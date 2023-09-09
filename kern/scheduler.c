/**
 * @file scheduler.c
 * @author Tony Xi (xiaolix)
 * @author Zekun Ma (zekunm)
 * @brief implementation of schedulers
 */

#include <scheduler.h> // round_robin
#include <ctrl_blk.h> // threads_lists
#include <stddef.h> // NULL

/**
 * @brief Rotate the list of runnable threads so that each time this
 *        function is called, it tried to return a thread of a
 *        different process.
 * 
 * Please note that if there is no thread that belongs to a different
 * process, this function will not rotate the thread list.
 * 
 * This function should be called only when interrupts are disabled.
 * 
 * @return tcb_t* NULL if there is no runnable thread. The pointer to
 *                the TCB of a runnable thread otherwise.
 */
tcb_t *round_robin(void) {
    if (thread_lists[READY_STATE] == NULL) {
        return NULL;
    } else {
        tcb_t *tcb_ptr = thread_lists[READY_STATE]->data;

        tcb_ptr_node_t *node_ptr = thread_lists[READY_STATE]->next;
        while (node_ptr != thread_lists[READY_STATE]) {
            if (node_ptr->data->pcb_ptr != thread_lists[READY_STATE]->data->pcb_ptr) {
                thread_lists[READY_STATE] = node_ptr;
                break;
            }
            node_ptr = node_ptr->next;
        }

        return tcb_ptr;
    }
}

/**
 * @brief Try to find a runnable thread of the same process as the thread
 *        running now. If such a thread does not exist, return the result
 *        of round robin.
 * 
 * This function should be called only when PCB lock is held and
 * interrupts are disabled. 
 * 
 * @return tcb_t* NULL if there is no runnable thread. The pointer to
 *                the TCB of a runnable thread otherwise.
 */
tcb_t *find_next_thread(void) {
    pcb_t *current_process = thread_lists[RUNNING_STATE]->data->pcb_ptr;
    if (current_process->tcb_list != NULL) {
        tcb_node_t *node_ptr = current_process->tcb_list;
        do {
            if (node_ptr->data.state == READY_STATE) {
                return &(node_ptr->data);
            }
            node_ptr = node_ptr->next;
        } while (node_ptr != current_process->tcb_list);
    }
    return round_robin();
}
