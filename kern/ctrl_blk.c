/**
 * @file ctrl_blk.c
 * @author Tony Xi (xiaolix)
 * @author Zekun Ma (zekunm)
 * @brief manipulations on TCB and PCB 
 */

#include <ctrl_blk.h>
#include <simics.h>
#include <stddef.h>
#include <page.h>
#include <string.h>
#include <stdint.h>
#include <mutex.h>
#include <context.h>
#include <vm.h>
#include <asm.h>
#include <eflags.h>
#include <stdbool.h>

// Although root_pcb_node_ptr is a global variable,
// it may not change once fixed. In the sense that
// it is read only, we don't need to lock before
// accessing it.
pcb_node_t *root_pcb_node_ptr = NULL;
// thread_count should be used only when
// thread_manager_lock is held.
int thread_count = 0;
mutex_t thread_manager_lock;
// Normally only thread_lists[RUNNING_STATE] can be
// observed with interrupts enabled. Please note that
// every list in thread_lists keeps elements in its
// own order. alter_state is the official way of
// modifying thread_lists.
tcb_ptr_node_t *thread_lists[THREAD_LIST_COUNT] = {NULL};

/**
 * @brief initialize the internal bookkeeping for TCBs and PCBs
 * 
 * @return a negative value on failure, 0 otherwise
 */
int init_ctrl_blk(void) {
    bool success;
    PUSH_FRONT(
        pcb_node_t,
        root_pcb_node_ptr,
        ((pcb_t){
            .page_directory = construct_page_dir()
        }),
        success
    );
    if (!success) {
        return -1;
    }
    PUSH_FRONT(
        tcb_node_t,
        root_pcb_node_ptr->data.tcb_list,
        ((tcb_t){
            .tid = thread_count++,
            .pcb_ptr = &(root_pcb_node_ptr->data),
            .state = RUNNING_STATE
        }),
        success
    );
    if (!success) {
        POP_BACK(pcb_node_t, root_pcb_node_ptr);
        return -1;
    }
    mutex_init(&(root_pcb_node_ptr->data.lock));
    PUSH_FRONT(
        tcb_ptr_node_t,
        thread_lists[RUNNING_STATE],
        &(root_pcb_node_ptr->data.tcb_list->data),
        success
    );
    if (!success) {
        POP_BACK(tcb_node_t, root_pcb_node_ptr->data.tcb_list);
        POP_BACK(pcb_node_t, root_pcb_node_ptr);
        return -1;
    }

    mutex_init(&thread_manager_lock);

    lprintf("The first TCB has been set up.");
    return 0;
}

// thread_fork_ctrl_blk should be called only when PCB lock is held.
int thread_fork_ctrl_blk(ureg_t *ureg_ptr) {
    if (ureg_ptr == NULL) {
        return -1;
    }

    tcb_t *old_tcb_ptr = thread_lists[RUNNING_STATE]->data;
    pcb_t *pcb_ptr = old_tcb_ptr->pcb_ptr;
    bool success;
    PUSH_BACK(
        tcb_node_t,
        pcb_ptr->tcb_list,
        *old_tcb_ptr,
        success
    );
    if (!success) {
        return -1;
    }
    tcb_t *new_tcb_ptr = &(pcb_ptr->tcb_list->previous->data);

    new_tcb_ptr->exception_stack = NULL;
    new_tcb_ptr->state = READY_STATE;
    mutex_lock(&thread_manager_lock);
    int new_tid = thread_count++;
    mutex_unlock(&thread_manager_lock);
    new_tcb_ptr->tid = new_tid;

    save_ureg(new_tcb_ptr, ureg_ptr);

    tcb_ptr_node_t *new_node_ptr = (tcb_ptr_node_t *)malloc(
        sizeof(tcb_ptr_node_t)
    );
    if (new_node_ptr == NULL) {
        POP_BACK(tcb_node_t, pcb_ptr->tcb_list);
        return -1;
    }
    disable_interrupts();
    new_node_ptr->data = new_tcb_ptr;
    bool process_found = false;
    if (thread_lists[READY_STATE] != NULL) {
        tcb_ptr_node_t *node_ptr = thread_lists[READY_STATE]->previous;
        do {
            if (node_ptr->data->pcb_ptr == pcb_ptr) {
                new_node_ptr->next = node_ptr->next;
                new_node_ptr->previous = node_ptr;
                new_node_ptr->next->previous = new_node_ptr;
                new_node_ptr->previous->next = new_node_ptr;
                process_found = true;
                break;
            }
            node_ptr = node_ptr->previous;
        } while (node_ptr != thread_lists[READY_STATE]->previous);
    }
    if (!process_found) {
        if (thread_lists[READY_STATE] == NULL) {
            new_node_ptr->next = new_node_ptr;
            new_node_ptr->previous = new_node_ptr;
            thread_lists[READY_STATE] = new_node_ptr;
        } else {
            new_node_ptr->next = thread_lists[READY_STATE];
            new_node_ptr->previous = thread_lists[READY_STATE]->previous;
            new_node_ptr->next->previous = new_node_ptr;
            new_node_ptr->previous->next = new_node_ptr;
        }
    }
    enable_interrupts();

    return new_tid;
}

/**
 * @brief do fork on internal bookkeeping, i.e., create a process
 *        that is exactly the same as the current one
 * 
 * This function should be called only in a single-threading process.
 * 
 * @param ureg_ptr pointers to register values to be loaded right after
 *                 the kernel switches to the forked process
 * @return a negative value on failure, 0 on success
 */
int fork_ctrl_blk(ureg_t *ureg_ptr) {
    if (ureg_ptr == NULL) {
        return -1;
    }

    // --- Copying PCB starts. ---

    pde_t *child_process_pd = construct_page_dir();
    if (child_process_pd == NULL) {
        lprintf("page_dir malloc failed");
        return -1;
    }
    
    // prepare buffer for coping user pages
    char *buf = (char*)malloc(PAGE_SIZE);
    if(buf == NULL) {
        destruct_page_dir(child_process_pd);
        lprintf("buf malloc failed");
        return -1;
    }

    tcb_t *old_tcb_ptr = thread_lists[RUNNING_STATE]->data;
    pcb_t *parent_pcb_ptr = old_tcb_ptr->pcb_ptr;
    bool success;
    PUSH_BACK(
        pcb_node_t,
        parent_pcb_ptr->child_pcb_list,
        ((pcb_t){
            .parent_pcb_ptr = parent_pcb_ptr,
            .page_directory = child_process_pd
        }),
        success
    );
    if (!success) {
        free(buf);
        destruct_page_dir(child_process_pd);
        lprintf("child pcb malloc failed");
        return -1;
    }
    pcb_t *child_pcb_ptr = &(parent_pcb_ptr->child_pcb_list->previous->data);
    mutex_init(&(child_pcb_ptr->lock));

    // copy user pages
    uint32_t parent_cr3 = get_cr3();
    pde_t *parent_process_pd = (pde_t *)((parent_cr3 >> PAGE_SHIFT) << PAGE_SHIFT);
    uint32_t child_cr3 = (parent_cr3 & ~((~0 >> PAGE_SHIFT) << PAGE_SHIFT)) |
        (uint32_t)child_process_pd;
    uint64_t current_page = USER_PAGE_START;
    while (current_page < VIRTUAL_ADDR_END) {
        mapping_info_t mapping_info;
        check_user_page(parent_process_pd, current_page, &mapping_info);
        switch (mapping_info) {
            case PDE_NOT_PRESENT: {
                uint64_t pde_idx = (current_page >> PAGE_SHIFT) / PTE_COUNT;
                uint64_t next_page = ((pde_idx + 1) * PTE_COUNT) << PAGE_SHIFT;

                uint32_t availability;
                get_availability(parent_process_pd, current_page, &availability);
                if (availability != PAGE_UNAVAILABLE) {
                    for (uint64_t i = current_page; i < next_page; i += PAGE_SIZE) {
                        if ((set_availability(child_process_pd, i, availability)) < 0) {
                            POP_BACK(pcb_node_t, parent_pcb_ptr->child_pcb_list);
                            free(buf);
                            destruct_page_dir(child_process_pd);
                            return -1;
                        }
                    }
                }
                
                current_page = next_page;
                break;
            }
            case PTE_NOT_PRESENT: {
                uint32_t availability;
                get_availability(parent_process_pd, current_page, &availability);
                if (set_availability(
                    child_process_pd,
                    current_page,
                    availability
                ) < 0) {
                    POP_BACK(pcb_node_t, parent_pcb_ptr->child_pcb_list);
                    free(buf);
                    destruct_page_dir(child_process_pd);
                    return -1;
                }
                current_page += PAGE_SIZE;
                break;
            }
            case ZERO_FRAME_MAPPED: {
                if (
                    set_availability(
                        child_process_pd,
                        current_page,
                        PAGE_AVAILABLE
                    ) < 0 ||
                    map_zero_frame(
                        child_process_pd,
                        current_page
                    ) < 0
                ) {
                    POP_BACK(pcb_node_t, parent_pcb_ptr->child_pcb_list);
                    free(buf);
                    destruct_page_dir(child_process_pd);
                    return -1;
                }
                current_page += PAGE_SIZE;
                break;
            }
            case NEW_FRAME_MAPPED: {
                if (
                    set_availability(
                        child_process_pd,
                        current_page,
                        PAGE_AVAILABLE
                    ) < 0 ||
                    map_new_frame(
                        child_process_pd,
                        current_page
                    ) < 0
                ) {
                    POP_BACK(pcb_node_t, parent_pcb_ptr->child_pcb_list);
                    free(buf);
                    destruct_page_dir(child_process_pd);
                    return -1;
                }

                memmove(buf, (void *)(uint32_t)current_page, PAGE_SIZE);
                parent_pcb_ptr->page_directory = child_process_pd;
                set_cr3(child_cr3);
                memmove((void *)(uint32_t)current_page, buf, PAGE_SIZE);
                parent_pcb_ptr->page_directory = parent_process_pd;
                set_cr3(parent_cr3);
                
                uint32_t access;
                get_access(parent_process_pd, current_page, &access);
                set_access(child_process_pd, current_page, access);

                current_page += PAGE_SIZE;
                break;
            }
            default: {
                POP_BACK(pcb_node_t, parent_pcb_ptr->child_pcb_list);
                free(buf);
                destruct_page_dir(child_process_pd);
                return -1;
            }
        }
    }

    free(buf);

    if (parent_pcb_ptr->page_allocation_list != NULL) {
        page_allocation_node_t *node_ptr =
            parent_pcb_ptr->page_allocation_list;
        do {
            PUSH_BACK(
                page_allocation_node_t,
                child_pcb_ptr->page_allocation_list,
                node_ptr->data,
                success
            );
            if (!success) {
                while (child_pcb_ptr->page_allocation_list != NULL) {
                    POP_FRONT(
                        page_allocation_node_t,
                        child_pcb_ptr->page_allocation_list
                    );
                }
                POP_BACK(pcb_node_t, parent_pcb_ptr->child_pcb_list);
                destruct_page_dir(child_process_pd);
                return -1;
            }
            node_ptr = node_ptr->next;
        } while (node_ptr != parent_pcb_ptr->page_allocation_list);
    }

    // --- Copying PCB ends. ---
    // --- Copying TCB starts. ---

    PUSH_FRONT(tcb_node_t, child_pcb_ptr->tcb_list, *old_tcb_ptr, success);
    if (!success) {
        while (child_pcb_ptr->page_allocation_list != NULL) {
            POP_FRONT(
                page_allocation_node_t,
                child_pcb_ptr->page_allocation_list
            );
        }
        POP_BACK(pcb_node_t, parent_pcb_ptr->child_pcb_list);
        destruct_page_dir(child_process_pd);
        return -1;
    }
    tcb_t *new_tcb_ptr = &(child_pcb_ptr->tcb_list->data);
    new_tcb_ptr->state = READY_STATE;
    new_tcb_ptr->pcb_ptr = child_pcb_ptr;
    mutex_lock(&thread_manager_lock);
    int new_tid = thread_count++;
    mutex_unlock(&thread_manager_lock);
    new_tcb_ptr->tid = new_tid;

    // make kernel stack for the new TCB
    save_ureg(new_tcb_ptr, ureg_ptr);

    // Set the new thread as runnable. From now on, the new thread
    // is ready for context switch.
    tcb_ptr_node_t *new_node_ptr = (tcb_ptr_node_t *)malloc(
        sizeof(tcb_ptr_node_t)
    );
    if (new_node_ptr == NULL) {
        POP_BACK(tcb_node_t, child_pcb_ptr->tcb_list);
        while (child_pcb_ptr->page_allocation_list != NULL) {
            POP_FRONT(
                page_allocation_node_t,
                child_pcb_ptr->page_allocation_list
            );
        }
        POP_BACK(pcb_node_t, parent_pcb_ptr->child_pcb_list);
        destruct_page_dir(child_process_pd);
        return -1;
    }
    bool interrupt_enable_flag = ((get_eflags() & EFL_IF) != 0);
    disable_interrupts();
    new_node_ptr->data = new_tcb_ptr;
    if (thread_lists[READY_STATE] == NULL) {
        new_node_ptr->previous = new_node_ptr;
        new_node_ptr->next = new_node_ptr;
        thread_lists[READY_STATE] = new_node_ptr;
    } else {
        new_node_ptr->previous = thread_lists[READY_STATE]->previous;
        new_node_ptr->next = thread_lists[READY_STATE];
        new_node_ptr->previous->next = new_node_ptr;
        new_node_ptr->next->previous = new_node_ptr;
    }
    if (interrupt_enable_flag) {
        enable_interrupts();
    }

    // --- Copying TCBs ends. ---

    sim_reg_child((void *)child_cr3, (void *)parent_cr3);
    return new_tid;
}

/**
 * @brief alter the state of a thread
 * 
 * This function should be called only when interrupts are disabled.
 * 
 * @param tcb_ptr pointer to the TCB of the thread
 * @param state the target state
 * @param blocking_detail_ptr pointer to the blocking reason, used
 *                            only when the target state is
 *                            WAITING_STATE
 * @return a negative value on failure, 0 on success
 */
int alter_state(
    tcb_t *tcb_ptr,
    int state,
    blocking_detail_t *blocking_detail_ptr
) {
    if (tcb_ptr == NULL || !(state >= READY_STATE && state <= TERMINATED_STATE)) {
        return -1;
    }
    if (state == WAITING_STATE && blocking_detail_ptr == NULL) {
        return -1;
    }

    // decide where the TCB pointer is in the original list
    tcb_ptr_node_t *source_node_ptr = NULL;
    int original_list_idx;
    if (tcb_ptr->state >= READY_STATE && tcb_ptr->state <= TERMINATED_STATE) {
        if (
            tcb_ptr->state != WAITING_STATE || (
                tcb_ptr->blocking_detail.reason > TERMINATED_STATE &&
                tcb_ptr->blocking_detail.reason < THREAD_LIST_COUNT
            )
        ) {
            original_list_idx = (tcb_ptr->state == WAITING_STATE) ?
                tcb_ptr->blocking_detail.reason :
                tcb_ptr->state;
            if (thread_lists[original_list_idx] != NULL) {
                tcb_ptr_node_t *node_ptr = thread_lists[original_list_idx];
                do {
                    if (node_ptr->data == tcb_ptr) {
                        source_node_ptr = node_ptr;
                        break;
                    }
                    node_ptr = node_ptr->next;
                } while (node_ptr != thread_lists[original_list_idx]);
            }
        }
    }
    if (source_node_ptr == NULL) {
        return -1;
    }
        
    // decide where the TCB pointer will be in the target list
    tcb_ptr_node_t *destination_node_ptr;
    bool front_pushed;
    int target_list_idx;
    switch (state) {
        case WAITING_STATE: {
            // For the thread list of waiting state, we actually use
            // thread lists of different blocking reasons instead.
            int reason = blocking_detail_ptr->reason;
            switch (reason) {
                case SLEEP: {
                    // For the thread list of SLEEP blocking reason,
                    // we sort it by wake up time.
                    unsigned int wakeup_time =
                        blocking_detail_ptr->wakeup_time;
                    if (
                        thread_lists[reason] == NULL
                        ||
                        thread_lists[reason]->data->blocking_detail.
                            wakeup_time >= wakeup_time
                    ) {
                        destination_node_ptr = thread_lists[reason];
                        front_pushed = true;
                        target_list_idx = reason;
                    } else {
                        tcb_ptr_node_t *node_ptr =
                            thread_lists[reason]->next;
                        while (
                            node_ptr != thread_lists[reason] &&
                            node_ptr->data->blocking_detail.wakeup_time <
                                wakeup_time
                        ) {
                            node_ptr = node_ptr->next;
                        }
                        destination_node_ptr = node_ptr;
                        front_pushed = false;
                        target_list_idx = reason;
                    }
                    break;
                }
                case DESCHEDULE: {
                    destination_node_ptr = thread_lists[reason];
                    front_pushed = false;
                    target_list_idx = reason;
                    break;
                }
                case VANISH_WAIT: {
                    destination_node_ptr = thread_lists[reason];
                    front_pushed = false;
                    target_list_idx = reason;
                    break;
                }
                case READLINE: {
                    // For the thread list of READLINE blocking reason,
                    // we prepend the TCB pointer to the list if first_reader
                    // is true and append the TCB pointer otherwise.
                    bool first_reader = blocking_detail_ptr->first_reader;
                    if (first_reader) {
                        destination_node_ptr = thread_lists[reason];
                        front_pushed = true;
                        target_list_idx = reason;
                    } else {
                        destination_node_ptr = thread_lists[reason];
                        front_pushed = false;
                        target_list_idx = reason;
                    }
                    break;
                }
                default: {
                    return -1;
                }
            }
            break;
        }
        case READY_STATE: {
            // For the thread list of ready state, put threads of the same
            // process together.
            bool process_found = false;
            if (thread_lists[state] != NULL) {
                tcb_ptr_node_t *node_ptr = thread_lists[state]->previous;
                do {
                    if (node_ptr->data->pcb_ptr == tcb_ptr->pcb_ptr) {
                        destination_node_ptr = node_ptr->next;
                        front_pushed = false;
                        target_list_idx = state;
                        process_found = true;
                        break;
                    }
                    node_ptr = node_ptr->previous;
                } while (node_ptr != thread_lists[state]->previous);
            }
            if (!process_found) {
                destination_node_ptr = thread_lists[state];
                front_pushed = false;
                target_list_idx = state;
            }
            break;
        }
        case RUNNING_STATE: {
            // For the thread list of running state, prepend the TCB
            // pointer to the list.
            destination_node_ptr = thread_lists[state];
            front_pushed = true;
            target_list_idx = state;
            break;
        }
        case TERMINATED_STATE: {
            // For the thread list of terminated state, append the TCB
            // pointer to the list.
            destination_node_ptr = thread_lists[state];
            front_pushed = false;
            target_list_idx = state;
            break;
        }
        case NEW_STATE:
        default: {
            return -1;
        }
    }

    if (source_node_ptr == thread_lists[original_list_idx]) {
        if (
            thread_lists[original_list_idx] ==
            thread_lists[original_list_idx]->next
        ) {
            thread_lists[original_list_idx] = NULL;
        } else {
            thread_lists[original_list_idx] =
            thread_lists[original_list_idx]->next;
        }
    }
    source_node_ptr->next->previous = source_node_ptr->previous;
    source_node_ptr->previous->next = source_node_ptr->next;
    if (destination_node_ptr == NULL) {
        source_node_ptr->next = source_node_ptr;
        source_node_ptr->previous = source_node_ptr;
        thread_lists[target_list_idx] = source_node_ptr;
    } else {
        source_node_ptr->next = destination_node_ptr;
        source_node_ptr->previous = destination_node_ptr->previous;
        source_node_ptr->next->previous = source_node_ptr;
        source_node_ptr->previous->next = source_node_ptr;
        if (front_pushed) {
            thread_lists[target_list_idx] = source_node_ptr;
        }
    }

    if (state == WAITING_STATE) {
        tcb_ptr->blocking_detail = *blocking_detail_ptr;
    }
    tcb_ptr->state = state;

    return 0;
}

/**
 * @brief Count how many threads of the process are not in TERMINATED_STATE.
 * 
 * This function should be called only when PCB lock is held.
 * 
 * @param pcb_ptr pointer to PCB of the process
 * @param thread_alive_count_ptr where the count will be stored
 * @return a negative value on failure, 0 on success
 */
int get_thread_alive_count(pcb_t *pcb_ptr, int *thread_alive_count_ptr) {
    if (pcb_ptr == NULL || thread_alive_count_ptr == NULL) {
        return -1;
    }
    int thread_alive_count = 0;
    if (pcb_ptr->tcb_list != NULL) {
        tcb_node_t *node_ptr = pcb_ptr->tcb_list;
        do {
            if (node_ptr->data.state != TERMINATED_STATE) {
                thread_alive_count++;
            }
            node_ptr = node_ptr->next;
        } while (node_ptr != pcb_ptr->tcb_list);
    }
    *thread_alive_count_ptr = thread_alive_count;
    return 0;
}

tcb_t *find_tcb_in_list(void *value, int state){
    tcb_ptr_node_t *current = thread_lists[state];
    if (current == NULL)
    {
        return NULL;
    }
    
    do
    {
        tcb_t *tcb_ptr = current->data;
        pcb_t *current_pcb = tcb_ptr->pcb_ptr;
        if (state == VANISH_WAIT)
        {
            pcb_t *parent_pcb = (pcb_t*) value;
            if (current_pcb == parent_pcb)
            {
                return tcb_ptr;
            }
        }else{
            int tid = (int) value;
            if (tcb_ptr->tid == tid)
            {
                return tcb_ptr;
            }
        }
        current = current->next;
    } while (current != thread_lists[state]);
    
    return NULL;
}

pcb_node_t *find_exited_child(pcb_t *parent_pcb){
    pcb_node_t *current = parent_pcb->child_pcb_list;
    if (current == NULL)
    {
        return NULL;
    }
    do
    {
        int alive_count;
        pcb_t *current_child = &(current->data);
        // lprintf("locking child %p", current);
        mutex_lock(&(current_child->lock));
        // lprintf("locked child lock %p", &(current_child->lock));
        affirm(!get_thread_alive_count(current_child, &alive_count));
        // lprintf("child alive count %d", alive_count);
        if (alive_count == 0)
        {
            return current;
        }

        current = current->next;
    } while (current != parent_pcb->child_pcb_list);

    return NULL;
}

int unlock_children(pcb_t *pcb, pcb_node_t *node){
    pcb_node_t *current = pcb->child_pcb_list;
    pcb_node_t *last_node = node!=NULL ? node->next : pcb->child_pcb_list;
    if (current){
        do
        {
            // lprintf("unlocking child %p", &(current->data.lock));
            mutex_unlock(&(current->data.lock));
            current = current->next;
        } while (current != last_node);
    }
    return 0;
}