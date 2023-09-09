/**
 * @file fault_handler.c
 * @author Tony Xi (xiaolix)
 * @author Zekun Ma (zekunm)
 * @brief implmentation of fault handlers
 */

#include <fault_handler.h> // handle_page_fault
#include <ureg.h> // ureg_t
#include <stdint.h> // uint32_t
#include <vm.h> // map_zero_frame
#include <simics.h> // lprintf
#include <cr.h> // get_cr3
#include <stdbool.h> // bool
#include <string.h> // memset
#include <page.h> // PAGE_SHIFT
#include <system_call.h> // handle_task_vanish
#include <ctrl_blk.h> // tcb_t
#include <mutex.h> // mutex_lock

/**
 * @brief Kernel decides to kill the thread. If the thread is the
 *        last one of its process, set status to -2.
 */
void fault_kill_thread(void) {
    pcb_t *pcb_ptr = thread_lists[RUNNING_STATE]->data->pcb_ptr;
    mutex_lock(&(pcb_ptr->lock));
    int thread_alive_count;
    get_thread_alive_count(pcb_ptr, &thread_alive_count);
    mutex_unlock(&(pcb_ptr->lock)); 

    if (thread_alive_count <= 1) {
        pcb_ptr->status = -2;
    }
    ureg_t ureg = {.cause = 0};
    handle_vanish(&ureg);
}

/**
 * @brief handle page fault
 * 
 * @param ureg_ptr pointer to the register values snapshotted before
 *                 the interrupt happens
 */
void handle_page_fault(ureg_t *ureg_ptr) {
    pcb_t *current_pcb_ptr = thread_lists[RUNNING_STATE]->data->pcb_ptr;
    mutex_lock(&(current_pcb_ptr->lock));

    int p = ureg_ptr->error_code & 1;
    int wr = (ureg_ptr->error_code >> 1) & 1;
    uint32_t v_addr = ureg_ptr->cr2;

    uint32_t page = (v_addr >> PAGE_SHIFT) << PAGE_SHIFT;
    pde_t * page_dir = (pde_t*) ((get_cr3() >> PAGE_SHIFT) << PAGE_SHIFT);

    if (p == 0) {
        // Either the PDE or the PTE is not present, in which case the page
        // must be in user address space.
        uint32_t availability;
        if (
            !(get_availability(page_dir, v_addr, &availability) < 0) &&
            availability == PAGE_AVAILABLE
        ) {
            // The available bits are PAGE_AVAILABLE, which means we can
            // map the page to a physical frame.
            if (wr == 0) {
                // map the zero frame on read operation
                if (!(map_zero_frame(page_dir, v_addr) < 0)) {
                    mutex_unlock(&(current_pcb_ptr->lock));
                    return;
                }
            } else {
                // map a newly allocated frame on write operation
                if (!(map_new_frame(page_dir, v_addr) < 0)) {
                    memset((void *)page, 0, PAGE_SIZE);
                    mutex_unlock(&(current_pcb_ptr->lock));
                    return;
                }
            }
        }
    } else {
        // The page has been mapped to a physical frame, which means
        // the fault is due to privilege (U/S bits) mismatch or
        // access (R/W bit) mismatch.
        mapping_info_t mapping_info;
        if (
            !(check_user_page(page_dir, v_addr, &mapping_info) < 0) &&
            mapping_info == ZERO_FRAME_MAPPED &&
            !(unmap_frame(page_dir, v_addr) < 0) &&
            !(map_new_frame(page_dir, v_addr) < 0)
        ) {
            // The first time the user progam writes to a page mapped
            // to the zero frame, we allocate a new frame and remap.
            memset((void *)page, 0, PAGE_SIZE);
            mutex_unlock(&(current_pcb_ptr->lock));
            return;
        }
    }

    mutex_unlock(&(current_pcb_ptr->lock));
    lprintf("Failed due to page fault.");
    fault_kill_thread();
}

/**
 * @brief handle segment fault
 * 
 * @param ureg_ptr pointer to the register values snapshotted before
 *                 the interrupt happens
 */
void handle_seg_fault(ureg_t *ureg_ptr) {
    lprintf("Failed due to seg fault.");
    fault_kill_thread();
}

/**
 * @brief handle division by zero
 * 
 * @param ureg_ptr pointer to the register values snapshotted before
 *                 the interrupt happens
 */
void handle_div_zero_fault(ureg_t *ureg_ptr) {
    lprintf("Failed due to division fault.");
    fault_kill_thread();
}
