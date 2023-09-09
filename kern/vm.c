#include <vm.h> // pde_t
#include <stdint.h> // uint32_t
#include <list.h> // PUSH_BACK
#include <page.h> // PAGE_SIZE
#include <malloc.h> // smemalign
#include <stddef.h> // NULL
#include <string.h> // memset
#include <stdbool.h> // bool
#include <simics.h> // lprintf
#include <mutex.h> // mutex_t
#include <common_kern.h> // machine_phys_frames

#define ZERO_FRAME (USER_PAGE_START)

typedef enum lookup_result_t { 
    NULL_PAGE_DIR,
    NONPRESENT_PDE,
    NONPRESENT_PTE,
    PHYSICAL_FRAME_MAPPED
} lookup_result_t;

DEFINE_NODE_T(frame_node, uint32_t);

/**
 * @brief This function attempts to get the PDE, the PTE and
 *        the physical frame related to a virtual address.
 *
 * @param page_dir The page directory.
 * @param v_addr The virtual address.
 * @param pde_ptr_holder To hold the PDE pointer if not NULL.
 *                       Used only when the page directory is
 *                       allocated.
 * @param pte_ptr_holder To hold the PTE pointer if not NULL.
 *                       Used only when the page directory is
 *                       allocated and the PDE is present.
 * @param p_addr_holder To hold the physical address of the frame
 *                      if not NULL. Used only when the page
 *                      directory is allocated, both the PDE and
 *                      the PTE are present.
 * @return NULL_PAGE_DIR if the page directory is not allocated.
 *         NONPRESENT_PDE if the page directory is allocated
 *         but the PDE is not present.
 *         NONPRESENT_PTE if the page directory is allocated,
 *         the PDE is present but the PTE is not present.
 *         PHYSICAL_FRAME_MAPPED otherwise.
 */
lookup_result_t find_frame (
    pde_t *page_dir,
    uint32_t v_addr,
    pde_t **pde_ptr_holder,
    pte_t **pte_ptr_holder,
    uint32_t *p_addr_holder
);

// a pool of physical frames that can be allocated,
// sorted in ascending order
frame_node *alloc_list = NULL;
// manage VM bookkeeping
mutex_t vm_lock;

/**
 * @brief Initialize the physical frame allocator.
 * 
 * This function must be called before the physical frame allocator
 * is used.
 * 
 * @return A negative value on failure, 0 otherwise.
 */
int init_allocator(void);

/**
 * @brief Allocate a physical frame if any.
 * 
 * If allocation fails, the memory pointed to by p_addr_ptr will not
 * be modified.
 * 
 * No guarantee is provided as for the content of the allocated frame.
 * 
 * @param p_addr_ptr The pointer to the physical address of the
 *                   allocated frame.
 * @return A negative value on failure, 0 otherwise.
 */
int alloc_frame(uint32_t *p_addr_ptr);

/**
 * @brief De-allocate a physical frame if it was allocated by
 *        physical frame allocator before.
 * 
 * @param p_addr The physical address of the frame to be de-allocated.
 * @return A negative value on failure, 0 otherwise.
 */
int free_frame(uint32_t p_addr);

/**
 * @brief Get the zero frame (concerning ZFOD).
 * 
 * @return The physical address of the zero frame.
 */
uint32_t get_zero_frame(void);

int init_allocator(void) {
    mutex_init(&vm_lock);

    // track free frames
    int frame_count = machine_phys_frames() - USER_PAGE_START / PAGE_SIZE;
    if (frame_count <= 0) {
        return -1;
    }
    for (int i = 0; i < frame_count; i++)
    {
        uint32_t frame = USER_PAGE_START + i * PAGE_SIZE;
        if (frame == ZERO_FRAME) {
            continue;
        }
        bool success;
        PUSH_BACK(frame_node, alloc_list, frame, success);
        if (!success) {
            while (alloc_list != NULL) {
                POP_BACK(frame_node, alloc_list);
            }
            return -1;
        }
    }

    // zero out the zero frame
    memset((void *)(ZERO_FRAME), 0, PAGE_SIZE);

    lprintf("Physical frame allocator is initialized.");
    return 0;
}

int alloc_frame(uint32_t *p_addr_ptr){
    if (p_addr_ptr == NULL) {
        return -1;
    }
    
    mutex_lock(&vm_lock);
    if (alloc_list == NULL) {
        mutex_unlock(&vm_lock);
        return -1;
    }

    *p_addr_ptr = alloc_list->data;
    POP_FRONT(frame_node, alloc_list);
    mutex_unlock(&vm_lock);
    return 0;
}

int free_frame(uint32_t p_addr){
    uint32_t frame = (p_addr >> PAGE_SHIFT) << PAGE_SHIFT;
    if (frame >= USER_PAGE_START && frame != ZERO_FRAME) {
        mutex_lock(&vm_lock);
        if (alloc_list == NULL || alloc_list->data > frame) {
            bool success;
            PUSH_FRONT(frame_node, alloc_list, frame, success);
            if (!success) {
                mutex_unlock(&vm_lock);
                return -1;
            }
        } else {
            frame_node *node = alloc_list;
            while (node->next != alloc_list && node->next->data <= frame) {
                node = node->next;
            }
            // double check if the frame exists in the pool
            if (node->data != frame) {
                node = node->next;
                bool success;
                PUSH_FRONT(frame_node, node, frame, success);
                if (!success) {
                    mutex_unlock(&vm_lock);
                    return -1;
                }
            }
        }
        mutex_unlock(&vm_lock);
    }
    return 0;
}

uint32_t get_zero_frame(void) {
    return ZERO_FRAME;
}

int init_page_dir_manager(void) {
    if (init_allocator() < 0) {
        return -1;
    }
    lprintf("Page directory manager is initialized.");
    return 0;
}

pde_t *construct_page_dir(void) {
    pde_t *page_dir = smemalign(PAGE_SIZE, PAGE_SIZE);
    if (page_dir == NULL) {
        return NULL;
    }

    uint32_t kernel_page_count = USER_PAGE_START / PAGE_SIZE;
    uint32_t kernel_page_table_count = (kernel_page_count + PTE_COUNT - 1) /
        PTE_COUNT;

    for (uint32_t i = 0; i < kernel_page_table_count; i++) {
        pte_t *pt = (pte_t *)smemalign(PAGE_SIZE, PAGE_SIZE);
        if (pt == NULL) {
            for (uint32_t j = 0; j < i; j++) {
                sfree((void *)(page_dir[j].pt_addr << PAGE_SHIFT), PAGE_SIZE);
            }
            sfree(page_dir, PAGE_SIZE);
            return NULL;
        }
        
        for (uint32_t j = 0; j < PTE_COUNT; j++) {
            uint32_t page_idx = i * PTE_COUNT + j;
            // With no pre-assumption, the kernel address space may not
            // be page directory aligned.
            if (page_idx < kernel_page_count) {
                pt[j] = (pte_t){
                    .page_addr = page_idx,
                    .p = 1,
                    .g = 1,
                    .rw = READ_WRITE
                };
            } else {
                pt[j] = (pte_t){
                    .p = 0
                };
            }
        }
        bool user_page_involved = (i + 1) * PTE_COUNT > kernel_page_count;
        page_dir[i] = (pde_t){
            .pt_addr = ((uint32_t)pt) >> PAGE_SHIFT,
            .g = user_page_involved ? 0 : 1,
            .us = user_page_involved ? 1 : 0,
            .p = 1,
            .rw = READ_WRITE
        };
    }

    for (uint32_t i = kernel_page_table_count; i < PDE_COUNT; i++) {
        page_dir[i] = (pde_t){
            .p = 0
        };
    }
    
    return page_dir;
}

void destruct_page_dir(pde_t *page_dir) {
    if (page_dir == NULL) {
        return;
    }
    uint32_t kernel_page_count = USER_PAGE_START / PAGE_SIZE;
    for (uint32_t i = 0; i < PDE_COUNT; i++) {
        if (page_dir[i].p == 1) {
            pte_t *page_table = (pte_t *)(page_dir[i].pt_addr << PAGE_SHIFT);
            for (uint32_t j = 0; j < PTE_COUNT; j++) {
                if (
                    page_table[j].p == 1 &&
                    (i * PTE_COUNT + j) >= kernel_page_count
                ) {
                    free_frame(page_table[j].page_addr << PAGE_SHIFT);
                }
            }
            sfree(page_table, PAGE_SIZE);
        }
    }
    sfree(page_dir, PAGE_SIZE);
}

int map_new_frame(pde_t *page_dir, uint32_t v_addr) {
    if (page_dir == NULL) {
        return -1;
    }
    uint32_t page = (v_addr >> PAGE_SHIFT) << PAGE_SHIFT;
    if (page < USER_PAGE_START) {
        return -1;
    }
    pde_t *pde_ptr;
    pte_t *pte_ptr;
    lookup_result_t lookup_result = find_frame(
        page_dir,
        v_addr,
        &pde_ptr,
        &pte_ptr,
        NULL
    );
    // Should be either NONPRESENT_PDE or NONPRESENT_PTE.
    // In the former case, a page table will be created.
    if (lookup_result == NONPRESENT_PDE) {
        pte_t *page_table = (pte_t *)smemalign(PAGE_SIZE, PAGE_SIZE);
        if (page_table == NULL) {
            return -1;
        }
        for (uint32_t i = 0; i < PTE_COUNT; i++) {
            // other PTEs inherit the PDE's available bits
            page_table[i] = (pte_t){
                .available = pde_ptr->available
            };
        }
        *pde_ptr = (pde_t){
            .pt_addr = ((uint32_t)page_table) >> PAGE_SHIFT,
            .us = 1,
            .p = 1,
            .rw = READ_WRITE
        };
        pte_ptr = &(page_table[(v_addr >> PAGE_SHIFT) % PTE_COUNT]);
    } else {
        if (lookup_result != NONPRESENT_PTE) {
            return -1;
        }
    }
    uint32_t p_addr;
    if (alloc_frame(&p_addr) < 0) {
        return -1;
    }
    *pte_ptr = (pte_t){
        .p = 1,
        .page_addr = p_addr >> PAGE_SHIFT,
        .us = 1,
        .rw = READ_WRITE
    };
    return 0;
}

int map_zero_frame(pde_t *page_dir, uint32_t v_addr) {
    if (page_dir == NULL) {
        return -1;
    }
    uint32_t page = (v_addr >> PAGE_SHIFT) << PAGE_SHIFT;
    if (page < USER_PAGE_START) {
        return -1;
    }
    pde_t *pde_ptr;
    pte_t *pte_ptr;
    lookup_result_t lookup_result = find_frame(
        page_dir,
        v_addr,
        &pde_ptr,
        &pte_ptr,
        NULL
    );
    // Should be either NONPRESENT_PDE or NONPRESENT_PTE.
    // In the former case, a page table will be created.
    if (lookup_result == NONPRESENT_PDE) {
        pte_t *page_table = (pte_t *)smemalign(PAGE_SIZE, PAGE_SIZE);
        if (page_table == NULL) {
            return -1;
        }
        for (uint32_t i = 0; i < PTE_COUNT; i++) {
            // other PTEs inherit the PDE's available bits
            page_table[i] = (pte_t){
                .available = pde_ptr->available
            };
        }
        *pde_ptr = (pde_t){
            .pt_addr = ((uint32_t)page_table) >> PAGE_SHIFT,
            .us = 1,
            .p = 1,
            .rw = READ_WRITE
        };
        pte_ptr = &(page_table[(v_addr >> PAGE_SHIFT) % PTE_COUNT]);
    } else {
        if (lookup_result != NONPRESENT_PTE) {
            return -1;
        }
    }
    *pte_ptr = (pte_t){
        .p = 1,
        .page_addr = get_zero_frame() >> PAGE_SHIFT,
        .us = 1,
        .rw = READ_ONLY
    };
    return 0;
}

int check_user_page(
    pde_t *page_dir,
    uint32_t v_addr,
    mapping_info_t *mapping_info_ptr
) {
    if (page_dir == NULL || mapping_info_ptr == NULL) {
        return -1;
    }
    uint32_t page = (v_addr >> PAGE_SHIFT) << PAGE_SHIFT;
    if (page < USER_PAGE_START) {
        return -1;
    }
    uint32_t p_addr;
    lookup_result_t lookup_result = find_frame(
        page_dir,
        v_addr,
        NULL,
        NULL,
        &p_addr
    );
    switch (lookup_result) {
        case NONPRESENT_PDE: {
            *mapping_info_ptr = PDE_NOT_PRESENT;
            break;
        }
        case NONPRESENT_PTE: {
            *mapping_info_ptr = PTE_NOT_PRESENT;
            break;
        }
        case PHYSICAL_FRAME_MAPPED: {
            if (p_addr == get_zero_frame()) {
                *mapping_info_ptr = ZERO_FRAME_MAPPED;
            } else {
                *mapping_info_ptr = NEW_FRAME_MAPPED;
            }
            break;
        }
        default: {
            return -1;
        }
    }
    return 0;
}

int unmap_frame(pde_t *page_dir, uint32_t v_addr) {
    if (page_dir == NULL) {
        return -1;
    }
    uint32_t page = (v_addr >> PAGE_SHIFT) << PAGE_SHIFT;
    if (page < USER_PAGE_START) {
        return -1;
    }
    pte_t *pte_ptr;
    uint32_t p_addr;
    if (find_frame(
        page_dir,
        v_addr,
        NULL,
        &pte_ptr,
        &p_addr
    ) != PHYSICAL_FRAME_MAPPED) {
        return -1;
    }
    // This page must have been PAGE_AVAILABLE before
    // it was allocated a physical frame.
    *pte_ptr = (pte_t){
        .available = PAGE_AVAILABLE
    };
    free_frame(p_addr);
    return 0;
}

int set_access(pde_t *page_dir, uint32_t v_addr, uint32_t access) {
    if (page_dir == NULL) {
        return -1;
    }
    uint32_t page = (v_addr >> PAGE_SHIFT) << PAGE_SHIFT;
    if (page < USER_PAGE_START) {
        return -1;
    }
    pte_t *pte_ptr;
    if (find_frame(
        page_dir,
        v_addr,
        NULL,
        &pte_ptr,
        NULL
    ) != PHYSICAL_FRAME_MAPPED) {
        return -1;
    }
    if (access != READ_ONLY && access != READ_WRITE) {
        return -1;
    }
    pte_ptr->rw = access;
    return 0;
}

int get_access(pde_t *page_dir, uint32_t v_addr, uint32_t *access_ptr) {
    if (page_dir == NULL || access_ptr == NULL) {
        return -1;
    }
    uint32_t page = (v_addr >> PAGE_SHIFT) << PAGE_SHIFT;
    if (page < USER_PAGE_START) {
        return -1;
    }
    pte_t *pte_ptr;
    if (find_frame(
        page_dir,
        v_addr,
        NULL,
        &pte_ptr,
        NULL
    ) != PHYSICAL_FRAME_MAPPED) {
        return -1;
    }
    *access_ptr = pte_ptr->rw;
    return 0;
}

int set_availability(pde_t *page_dir, uint32_t v_addr, uint32_t availability) {
    if (
        page_dir == NULL ||
        !(availability == PAGE_AVAILABLE || availability == PAGE_UNAVAILABLE)
    ) {
        return -1;
    }
    uint32_t page = (v_addr >> PAGE_SHIFT) << PAGE_SHIFT;
    if (page < USER_PAGE_START) {
        return -1;
    }
    pde_t *pde_ptr;
    pte_t *pte_ptr;
    lookup_result_t lookup_result = find_frame(
        page_dir,
        v_addr,
        &pde_ptr,
        &pte_ptr,
        NULL
    );
    if (lookup_result == NONPRESENT_PDE) {
        if (pde_ptr->available == availability) {
            return 0;
        }
        pte_t *page_table = (pte_t *)smemalign(PAGE_SIZE, PAGE_SIZE);
        if (page_table == NULL) {
            return -1;
        }
        for (uint32_t i = 0; i < PTE_COUNT; i++) {
            // other PTEs inherit the PDE's available bits
            page_table[i] = (pte_t){
                .available = pde_ptr->available
            };
        }
        *pde_ptr = (pde_t){
            .pt_addr = ((uint32_t)page_table) >> PAGE_SHIFT,
            .us = 1,
            .p = 1,
            .rw = READ_WRITE
        };
        pte_ptr = &(page_table[(v_addr >> PAGE_SHIFT) % PTE_COUNT]);
    } else {
        if (lookup_result != NONPRESENT_PTE) {
            return -1;
        }
    }
    pte_ptr->available = availability;
    return 0;
}

int get_availability(pde_t *page_dir, uint32_t v_addr, uint32_t *availability_ptr) {
    if (page_dir == NULL || availability_ptr == NULL) {
        return -1;
    }
    uint32_t page = (v_addr >> PAGE_SHIFT) << PAGE_SHIFT;
    if (page < USER_PAGE_START) {
        return -1;
    }
    pde_t *pde_ptr;
    pte_t *pte_ptr;
    lookup_result_t lookup_result = find_frame(
        page_dir,
        v_addr,
        &pde_ptr,
        &pte_ptr,
        NULL
    );
    switch (lookup_result) {
        case NONPRESENT_PDE: {
            // When the PDE is nonpresent, its available bits 
            // stand for all its PTE's.
            *availability_ptr = pde_ptr->available;
            break;
        }
        case NONPRESENT_PTE: {
            *availability_ptr = pte_ptr->available;
            break;
        }
        default: {
            return -1;
        }
    }
    return 0;
}

lookup_result_t find_frame(
    pde_t *page_dir,
    uint32_t v_addr,
    pde_t **pde_ptr_holder,
    pte_t **pte_ptr_holder,
    uint32_t *p_addr_holder
) {
    if (page_dir == NULL) {
        return NULL_PAGE_DIR;
    }
    uint32_t pde_idx = (v_addr >> PAGE_SHIFT) / PTE_COUNT;
    if (pde_ptr_holder != NULL) {
        *pde_ptr_holder = &(page_dir[pde_idx]);
    }
    if (page_dir[pde_idx].p == 0)
    {
        return NONPRESENT_PDE;
    }
    pte_t *pt = (pte_t *)(page_dir[pde_idx].pt_addr << PAGE_SHIFT);
    uint32_t pte_idx = (v_addr >> PAGE_SHIFT) % PTE_COUNT;
    if (pte_ptr_holder != NULL) {
        *pte_ptr_holder = &(pt[pte_idx]);
    }
    if (pt[pte_idx].p == 0)
    {
        return NONPRESENT_PTE;
    }
    if (p_addr_holder != NULL) {
        *p_addr_holder = pt[pte_idx].page_addr << PAGE_SHIFT;
    }
    return PHYSICAL_FRAME_MAPPED;
}
