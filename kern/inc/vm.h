/**
 * @file vm.h
 * @author Tony Xi (xiaolix)
 * @author Zekun Ma (zekunm)
 * @brief The virtual memory module, including a physical frame
 *        allocator (not exposed to outside) and a page directory
 *        manager (which interacts with the physical frame
 *        allocator and is publicly available).
 */

#ifndef VM_H_SEEN
#define VM_H_SEEN

#include <page.h> // PAGE_SIZE
#include <stdint.h> // uint32_t
#include <limits.h> // CHAR_BIT
#include <stdbool.h> // bool
#include <common_kern.h> // USER_MEM_START

// how large the entire virtual address space is
#define VIRTUAL_ADDR_END ((uint64_t)1 << (sizeof(void *) * CHAR_BIT))
// the first page of the user space
#define USER_PAGE_START ((USER_MEM_START + PAGE_SIZE - 1) / PAGE_SIZE * PAGE_SIZE)
// how many PTEs in a page table
#define PTE_COUNT (PAGE_SIZE / sizeof(pte_t))
// how many PDEs in a page directory
#define PDE_COUNT (VIRTUAL_ADDR_END / PAGE_SIZE / PTE_COUNT)

// read/write bit
#define READ_ONLY (0)
#define READ_WRITE (1)

// Available bits in PTEs and PDEs,
// when PTEs and PDEs are not present,
// specify whether they can have
// physical frames allocated on demand.
#define PAGE_UNAVAILABLE (0)
#define PAGE_AVAILABLE (1)

// page directory entry (PDE) structure
typedef struct pde_t {
    uint32_t p                  : 1;
    uint32_t rw                 : 1;
    uint32_t us                 : 1;
    uint32_t write_through      : 1;
    uint32_t pcd                : 1;
    uint32_t accessed           : 1;
    uint32_t reserved           : 1;
    uint32_t ps                 : 1;
    uint32_t g                  : 1;
    uint32_t available          : 3;
    uint32_t pt_addr            : 20;
} pde_t;

// page table entry (PTE) structure
typedef struct pte_t {
    uint32_t p                  : 1;
    uint32_t rw                 : 1;
    uint32_t us                 : 1;
    uint32_t write_through      : 1;
    uint32_t pcd                : 1;
    uint32_t accessed           : 1;
    uint32_t dirty              : 1;
    uint32_t pat                : 1;
    uint32_t g                  : 1;
    uint32_t available          : 3;
    uint32_t page_addr          : 20;
} pte_t;

// information that tells how far a page is
// from being mapped to a physical frame
typedef enum mapping_info_t {
    PDE_NOT_PRESENT,
    PTE_NOT_PRESENT,
    ZERO_FRAME_MAPPED,
    NEW_FRAME_MAPPED
} mapping_info_t;

/**
 * @brief Initialize the page directory manager
 * 
 * The new_frame_count_max global variable as well as all
 * the VM APIs depend on this function being called.
 * 
 * @return 0 on success, a negative value otherwise.
 */
int init_page_dir_manager(void);

/**
 * @brief Create a page directory and initialize it.
 * 
 * The page directory will do direct mapping for kernel address space
 * and leave user address space unmapped.
 * 
 * @return pde_t* The new page directory, which, on memory allocation
 *                failure, will be NULL.
 */
pde_t *construct_page_dir(void);

/**
 * @brief de-allocate the physical frames recorded in the page
 *        directory and free its PDEs and PTEs
 * 
 * @param page_dir The page directory.
 */
void destruct_page_dir(pde_t *page_dir);

/**
 * @brief If the virtual page is not mapped, allocate a physical
 *        frame and map the virtual page to it.
 * 
 * This function is used only for a user page. It may create a page
 * table if the PDE is not present. The mapped page will be assigned
 * read write access. 
 * 
 * @param page_dir The page directory.
 * @param v_addr The virtual address within the range of the virtual
 *               page.
 * @return A negative value on failure, 0 otherwise.
 */
int map_new_frame(pde_t *page_dir, uint32_t v_addr);

// Similar to map_new_frame, except that the zero frame is mapped.
int map_zero_frame(pde_t *page_dir, uint32_t v_addr);

/**
 * @brief Find out which kind of frame a user page is mapped to.
 * 
 * @param page_dir The page directory.
 * @param v_addr The virtual address within the range of the user
 *               page.
 * @param mapping_info_ptr Where the mapping information of the
 *                         user page will be stored.
 * @return A negative value if page_dir is NULL, or mapping_info_ptr
 *         is NULL, or the page is not a user page, in which case
 *         mapping_info_ptr will not be used. 0 otherwise.
 */
int check_user_page(
    pde_t *page_dir,
    uint32_t v_addr,
    mapping_info_t *mapping_info_ptr
);

/**
 * @brief If the virtual page is mapped to a previsouly allocated
 *        physical frame or the zero frame, unmap it. In the former
 *        case, the physical frame will also be de-allocated.
 * 
 * This function is used only for a user page. It will set the
 * available bits to PAGE_AVAILABLE in the end. It will not delete
 * the page table if the page table ends up with no present PTE.
 * 
 * @param page_dir The page directory.
 * @param v_addr The virtual address within the range of the virtual
 *               page.
 * @return A negative value on failure, 0 otherwise.
 */
int unmap_frame(pde_t *page_dir, uint32_t v_addr);

/**
 * @brief Define whether the virtual page is read only or writtable
 *        if it is mapped to a physical frame.
 * 
 * @param page_dir The page directory.
 * @param v_addr The virtual address within the range of the virtual
 *               page.
 * @param access READ_ONLY or READ_WRITE.
 * @return 0 if the virtual page is a user page and is mapped to a
 *         physical frame, in which case the function call succeeds.
 *         A negative value otherwise.
 */
int set_access(pde_t *page_dir, uint32_t v_addr, uint32_t access);

// get the read/write bit of a user page that has been mapped to a
// physical frame
int get_access(pde_t *page_dir, uint32_t v_addr, uint32_t *access_ptr);

/**
 * @brief Set the available bits of the virtual page, i.e., whether it can
 *        be mapped to a physical frame the first time it is accessed.
 * 
 * This function is used only for a user page that has not been mapped
 * to a physical frame. It will create a page table if the PDE is not
 * present with different available bits.
 * 
 * @param page_dir The page directory.
 * @param v_addr The virtual address within the range of the virtual page.
 * @param availability AVAILABLE or UNAVAILABLE.
 * @return A negative value if page_dir is NULL, availability is not a
 *         valid value, the virtual page is not a user page or the virtual
 *         page is mapped to a physical frame. 0 otherwise.
 */
int set_availability(pde_t *page_dir, uint32_t v_addr, uint32_t availability);

// get the available bits of a user page that has not been mapped to
// a physical frame
int get_availability(pde_t *page_dir, uint32_t v_addr, uint32_t *availability_ptr);

#endif // VM_H_SEEN
