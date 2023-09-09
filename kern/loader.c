/**
 * The 15-410 kernel project.
 * @name loader.c
 *
 * Functions for the loading
 * of user programs from binary 
 * files should be written in
 * this file. The function 
 * elf_load_helper() is provided
 * for your use.
 */
/*@{*/

/* --- Includes --- */

#include <string.h>
#include <malloc.h>
#include <exec2obj.h>
#include <loader.h>
#include <elf_410.h>
#include <stddef.h>
#include <vm.h>
#include <stdint.h>
#include <common_kern.h>
#include <ctrl_blk.h>
#include <simics.h>
#include <cr.h>
#include <seg.h>
#include <eflags.h>
#include <ureg.h>
#include <string.h>
#include <segmentation.h>
#include <hvcall.h>

// default user stack length, measured in double words
#define USER_STACK_LEN (0x10000)

int change_access(
    pde_t *page_dir,
    uint32_t addr,
    uint32_t size,
    uint32_t access
);
int change_availability(
    pde_t *page_dir,
    uint32_t addr,
    uint32_t size,
    uint32_t availability
);
int map_and_clear(uint32_t addr, uint32_t size, bool zero_frame);

/**
 * Copies data from a file into a buffer.
 *
 * @param filename   the name of the file to copy data from
 * @param offset     the location in the file to begin copying from
 * @param size       the number of bytes to be copied
 * @param buf        the buffer to copy the data into
 *
 * @return returns the number of bytes copied on succes; -1 on failure
 */
int getbytes(const char *filename, int offset, int size, char *buf) {
    /*
     * You fill in this function.
     */

    // search for the executable with the name specified by filename
    if (filename == NULL) {
        return -1;
    }
    int executable_idx = -1;
    for (int i = 0; i < exec2obj_userapp_count; i++) {
        if (strcmp(filename, exec2obj_userapp_TOC[i].execname) == 0) {
            executable_idx = i;
            break;
        }
    }
    if (!(executable_idx >= 0)) {
        return -1;
    }

    // check the range
    if (offset < 0 || size < 0) {
        return -1;
    }
    if (offset >= exec2obj_userapp_TOC[executable_idx].execlen) {
        return -1;
    }
    if (size > exec2obj_userapp_TOC[executable_idx].execlen - offset) {
        size = exec2obj_userapp_TOC[executable_idx].execlen - offset;
    }

    // fill in buf
    if (buf == NULL) {
        return -1;
    }
    memmove(
        buf,
        (void *)(
            (uint32_t)(exec2obj_userapp_TOC[executable_idx].execbytes) +
            offset
        ),
        size
    );

    return size;
}

/**
 * @brief Set up the user address space of the current process based
 *        on the executable and arguments of the entry point.
 * 
 * This function uses the virtual address space of the current process.
 * Please make sure paging (page directory manager and physical frame
 * allocator) works well in advance!
 * 
 * No side effect will be made if the function fails in the end.
 * 
 * This function should be called only in a single-threading process.
 * 
 * @param execname see the exec system call
 * @param argvec see the exec system call
 * @param ureg_ptr to store the register values for running the
 *                 executable
 * @return a negative value on failure, 0 otherwise
 */
int load_executable(char *execname, char **argvec, ureg_t *ureg_ptr) {
    //  fail fast
    if (execname == NULL || argvec == NULL || ureg_ptr == NULL) {
        return -1;
    }
    simple_elf_t simple_elf;
    if (
        elf_check_header(execname) != ELF_SUCCESS ||
        elf_load_helper(&simple_elf, execname) != ELF_SUCCESS
    ) {
        return -1;
    }
    pde_t *new_page_dir = construct_page_dir();
    if (new_page_dir == NULL) {
        return -1;
    }

    // Make a copy of arguments on kernel heap.
    // Once we start loading, we cannot get these original arguments easily.
    char *executable_name = malloc((strlen(execname) + 1) * sizeof(char));
    if (executable_name == NULL) {
        destruct_page_dir(new_page_dir);
        return -1;
    }
    strcpy(executable_name, execname);
    int argc = 0;
    while (argvec[argc] != NULL) {
        argc++;
    }
    char **arg_array = NULL;
    if (argc > 0) {
        arg_array = (char **)malloc(argc * sizeof(char *));
        if (arg_array == NULL) {
            free(executable_name);
            destruct_page_dir(new_page_dir);
            return -1;
        }
        for (int i = 0; i < argc; i++) {
            arg_array[i] = (char *)malloc(
                (strlen(argvec[i]) + 1) * sizeof(char)
            );
            if (arg_array[i] == NULL) {
                for (int j = 0; j < i; j++) {
                    free(arg_array[j]);
                }
                free(arg_array);
                free(executable_name);
                destruct_page_dir(new_page_dir);
                return -1;
            }
            strcpy(arg_array[i], argvec[i]);
        }
    }
    
    // switch to a new page directory before loading the executable
    // so that we have way back on failure
    pcb_t *current_pcb_ptr = thread_lists[RUNNING_STATE]->data->pcb_ptr;
    uint32_t old_cr3 = get_cr3();
    pde_t *old_page_dir = (pde_t *)((old_cr3 >> PAGE_SHIFT) << PAGE_SHIFT);
    uint32_t new_cr3 = (uint32_t)new_page_dir |
        (old_cr3 & ~((~0 >> PAGE_SHIFT) << PAGE_SHIFT));
    current_pcb_ptr->page_directory = new_page_dir;
    set_cr3(new_cr3);

    // Test if the executable is a guest.
    bool guest = simple_elf.e_txtstart < USER_MEM_START;

    // ---------- make user stack ----------

    uint32_t stack_high;
    uint32_t stack_low;
    if (guest) {
        // allocate all the frames at once
        if (
            change_availability(
                new_page_dir,
                USER_MEM_START,
                GUEST_MEM_SIZE,
                PAGE_AVAILABLE
            ) < 0 ||
            map_and_clear(
                USER_MEM_START,
                GUEST_MEM_SIZE,
                false
            ) < 0
        ) {
            current_pcb_ptr->page_directory = old_page_dir;
            set_cr3(old_cr3);
            for (int i = 0; i < argc; i++) {
                free(arg_array[i]);
            }
            free(arg_array);
            free(executable_name);
            destruct_page_dir(new_page_dir);
            return -1;
        }
        stack_high = USER_MEM_START + GUEST_MEM_SIZE;
        stack_low = stack_high - USER_STACK_LEN * sizeof(uint32_t);
    } else {
        // set pages for the user stack to be available before actually
        // allocating frames to them
        stack_high = (uint32_t)VIRTUAL_ADDR_END;
        stack_low = stack_high - USER_STACK_LEN * sizeof(uint32_t);
        if (change_availability(
            new_page_dir,
            stack_low,
            stack_high - stack_low,
            PAGE_AVAILABLE
        ) < 0) {
            current_pcb_ptr->page_directory = old_page_dir;
            set_cr3(old_cr3);
            for (int i = 0; i < argc; i++) {
                free(arg_array[i]);
            }
            free(arg_array);
            free(executable_name);
            destruct_page_dir(new_page_dir);
            return -1;
        }
    }

    // put arguments and return address of _main onto user stack
    uint32_t esp = stack_high;
    if (argc > 0) {
        // push each string indicated in argv
        for (int i = 0; i < argc; i++) {
            if (push_val(
                &esp,
                arg_array[i],
                (strlen(arg_array[i]) + 1) * sizeof(char)
            ) < 0) {
                current_pcb_ptr->page_directory = old_page_dir;
                set_cr3(old_cr3);
                for (int j = i; j < argc; j++) {
                    free(arg_array[j]);
                }
                free(arg_array);
                free(executable_name);
                destruct_page_dir(new_page_dir);
                return -1;
            }
            free(arg_array[i]);
            arg_array[i] = (char *)esp;
        }
        // push the argv array
        if (push_val(
            &esp,
            arg_array,
            argc * sizeof(char *)
        ) < 0) {
            current_pcb_ptr->page_directory = old_page_dir;
            set_cr3(old_cr3);
            free(arg_array);
            free(executable_name);
            destruct_page_dir(new_page_dir);
            return -1;
        }
        free(arg_array);
        arg_array = (char **)esp;
    }
    bool success = true;
    uint32_t return_addr = 0;
    if (
        push_val(
            &esp,
            &stack_low,
            sizeof(stack_low)
        ) < 0 ||
        push_val(
            &esp,
            &stack_high,
            sizeof(stack_high)
        ) < 0 ||
        push_val(
            &esp,
            &arg_array,
            sizeof(arg_array)
        ) < 0 ||
        push_val(
            &esp,
            &argc,
            sizeof(argc)
        ) < 0 ||
        push_val(
            &esp,
            &return_addr,
            sizeof(return_addr)
        ) < 0
    ) {
        success = false;
    }

    // leave the rest of the user stack mapped to zero frame
    if (success) {
        if (map_and_clear(
            stack_low,
            esp - stack_low,
            true
        ) < 0) {
            success = false;
        }
    }

    // ---------- load sections from ELF ----------

    if (guest) {
        // load .text for the guest
        if (success) {
            if (getbytes(
                executable_name,
                simple_elf.e_txtoff,
                simple_elf.e_txtlen,
                (char *)(USER_MEM_START + simple_elf.e_txtstart)
            ) < 0) {
                success = false;
            }
        }

        // load .rodata for the guest
        if (success && simple_elf.e_rodatlen > 0) {
            if (getbytes(
                executable_name,
                simple_elf.e_rodatoff,
                simple_elf.e_rodatlen,
                (char *)(USER_MEM_START + simple_elf.e_rodatstart)
            ) < 0) {
                success = false;
            }
        }

        // load .data for the guest
        if (success && simple_elf.e_datlen > 0) {
            if (getbytes(
                executable_name,
                simple_elf.e_datoff,
                simple_elf.e_datlen,
                (char *)(USER_MEM_START + simple_elf.e_datstart)
            ) < 0) {
                success = false;
            }
        }

        // No need to adjust the read write bits of guest pages. These pages
        // are for the guest kernel. Just like those for the host kernel,
        // they are open to any read write operation.
        
        // No need to load .bss. The frames for it have been allocated
        // and cleared.

        if (success) {
            *ureg_ptr = (ureg_t){
                .ds = SEGSEL_GUEST_KERNEL_DS,
                .es = SEGSEL_GUEST_KERNEL_DS,
                .fs = SEGSEL_GUEST_KERNEL_DS,
                .gs = SEGSEL_GUEST_KERNEL_DS,
                .ebx = GUEST_MEM_SIZE / PAGE_SIZE > 0 ?
                    GUEST_MEM_SIZE / PAGE_SIZE - 1 :
                    0,
                .ecx = GUEST_MEM_SIZE > 0 ? GUEST_MEM_SIZE - 1 : 0,
                .eax = GUEST_LAUNCH_EAX,
                .eip = simple_elf.e_entry,
                .cs = SEGSEL_GUEST_KERNEL_CS,
                .eflags = EFL_IF | EFL_RESV1,
                .ss = SEGSEL_GUEST_KERNEL_DS,
            };
        }
    } else {
        // set page availability for each section
        if (success) {
            if (
                change_availability(
                    new_page_dir,
                    simple_elf.e_txtstart,
                    simple_elf.e_txtlen,
                    PAGE_AVAILABLE
                ) < 0 ||
                change_availability(
                    new_page_dir,
                    simple_elf.e_rodatstart,
                    simple_elf.e_rodatlen,
                    PAGE_AVAILABLE
                ) < 0 ||
                change_availability(
                    new_page_dir,
                    simple_elf.e_datstart,
                    simple_elf.e_datlen,
                    PAGE_AVAILABLE
                ) < 0 ||
                change_availability(
                    new_page_dir,
                    simple_elf.e_bssstart,
                    simple_elf.e_bsslen,
                    PAGE_AVAILABLE
                ) < 0
            ) {
                success = false;
            }
        }

        // load .text
        if (success) {
            if (
                map_and_clear(
                    simple_elf.e_txtstart,
                    simple_elf.e_txtlen,
                    false
                ) < 0 ||
                getbytes(
                    executable_name,
                    simple_elf.e_txtoff,
                    simple_elf.e_txtlen,
                    (char *)(simple_elf.e_txtstart)
                ) < 0
            ) {
                success = false;
            }
        }

        // load .rodata
        if (success && simple_elf.e_rodatlen > 0) {
            if (
                map_and_clear(
                    simple_elf.e_rodatstart,
                    simple_elf.e_rodatlen,
                    false
                ) < 0 ||
                getbytes(
                    executable_name,
                    simple_elf.e_rodatoff,
                    simple_elf.e_rodatlen,
                    (char *)(simple_elf.e_rodatstart)
                ) < 0
            ) {
                success = false;
            }
        }

        // load .data
        if (success && simple_elf.e_datlen > 0) {
            if (
                map_and_clear(
                    simple_elf.e_datstart,
                    simple_elf.e_datlen,
                    false
                ) < 0 ||
                getbytes(
                    executable_name,
                    simple_elf.e_datoff,
                    simple_elf.e_datlen,
                    (char *)(simple_elf.e_datstart)
                ) < 0
            ) {
                success = false;
            }
        }

        // unset the R/W bit of each PTE of read only sections,
        // then set the R/W bit of each PTE of read write sections
        if (success) {
            if (
                change_access(
                    new_page_dir,
                    simple_elf.e_txtstart,
                    simple_elf.e_txtlen,
                    READ_ONLY
                ) < 0 ||
                change_access(
                    new_page_dir,
                    simple_elf.e_rodatstart,
                    simple_elf.e_rodatlen,
                    READ_ONLY
                ) < 0 ||
                change_access(
                    new_page_dir,
                    simple_elf.e_datstart,
                    simple_elf.e_datlen,
                    READ_WRITE
                ) < 0
            ) {
                success = false;
            }
        }

        // load .bss
        if (success && simple_elf.e_bsslen > 0) {
            if (map_and_clear(
                simple_elf.e_bssstart,
                simple_elf.e_bsslen,
                true
            ) < 0) {
                success = false;
            }
        }

        if (success) {
            *ureg_ptr = (ureg_t){
                .ds = SEGSEL_USER_DS,
                .es = SEGSEL_USER_DS,
                .fs = SEGSEL_USER_DS,
                .gs = SEGSEL_USER_DS,
                .eip = simple_elf.e_entry,
                .cs = SEGSEL_USER_CS,
                .eflags = EFL_IF | EFL_RESV1,
                .esp = esp,
                .ss = SEGSEL_USER_DS,
            };
        }
    }

    // on failure, back off
    if (!success) {
        current_pcb_ptr->page_directory = old_page_dir;
        set_cr3(old_cr3);
        free(executable_name);
        destruct_page_dir(new_page_dir);
        return -1;
    }

    destruct_page_dir(old_page_dir);
    while (current_pcb_ptr->page_allocation_list != NULL) {
        POP_FRONT(
            page_allocation_node_t,
            current_pcb_ptr->page_allocation_list
        );
    }
    current_pcb_ptr->guest = guest;

    sim_reg_process((void *)new_cr3, executable_name);
    lprintf("%s is loaded.", executable_name);
    free(executable_name);
    return 0;
}

/**
 * @brief walk through a range of memory and change the read/write bit of each
 *        PTE
 * 
 * Even if this function fails, some PTEs may still have been modified.
 * 
 * @param page_dir the page directory
 * @param addr memory start
 * @param size memory size
 * @param access the desired read/write bit
 * @return a negative value on failure, 0 otherwise
 */
int change_access(
    pde_t *page_dir,
    uint32_t addr,
    uint32_t size,
    uint32_t access
) {
    if (size > 0) {
        for (
            uint64_t i = (addr >> PAGE_SHIFT) << PAGE_SHIFT;
            i < (uint64_t)addr + (uint64_t)size;
            i += PAGE_SIZE
        ) {
            if (set_access(page_dir, i, access) < 0) {
                return -1;
            }
        }
    }
    return 0;
}

// Similar to change_access, except that the available bits are to be changed.
int change_availability(
    pde_t *page_dir,
    uint32_t addr,
    uint32_t size,
    uint32_t availability
) {
    if (size > 0) {
        for (
            uint64_t i = (addr >> PAGE_SHIFT) << PAGE_SHIFT;
            i < (uint64_t)addr + (uint64_t)size;
            i += PAGE_SIZE
        ) {
            if (set_availability(page_dir, i, availability) < 0) {
                return -1;
            }
        }
    }
    return 0;
}

/**
 * @brief treat a pointer like a stack pointer and push a value to
 *        the memory pointed to by it
 * 
 * Unsigned extension will be done if size is not stack aligned.
 * The stack must belong to the user address space.
 * 
 * @param esp_ptr what is deemed as a stack pointer
 * @param val_ptr value to be pushed
 * @param size size of the value
 * @return a negative value on failure, 0 on success
 */
int push_val(uint32_t *esp_ptr, void *val_ptr, uint32_t size) {
    if (esp_ptr == NULL || val_ptr == NULL ) {
        return -1;
    }    
    if (size > 0) {
        uint32_t new_esp = (*esp_ptr - size) /
            sizeof(uint32_t) * sizeof(uint32_t);
        if (map_and_clear(new_esp, *esp_ptr - new_esp, false) < 0) {
            return -1;
        }
        for (uint32_t i = 0; i < size; i++) {
            ((uint8_t *)new_esp)[i] = ((uint8_t *)val_ptr)[i];
        }
        for (uint32_t i = new_esp + size; i < *esp_ptr; i++) {
            *(uint8_t *)i = 0;
        }
        *esp_ptr = new_esp;
    }
    return 0;
}

/**
 * @brief walk through a range of user memory and map any unmapped
 *        page to a physical frame
 * 
 * If any unmapped page is not available, this function will fail.
 * In the case it fails, part of the memory may still have been
 * mapped.
 * 
 * @param addr memory start
 * @param size memory size
 * @param zero_frame whether the unmapped pages will be mapped to
 *                   the zero frame or a newly allocated frame
 * @return a negative value on failure, 0 on success
 */
int map_and_clear(uint32_t addr, uint32_t size, bool zero_frame) {
    if (size > 0) {
        pde_t *page_dir = (pde_t *)((get_cr3() >> PAGE_SHIFT) << PAGE_SHIFT);
        for (
            uint64_t i = (addr >> PAGE_SHIFT) << PAGE_SHIFT;
            i < (uint64_t)addr + (uint64_t)size;
            i += PAGE_SIZE
        ) {
            mapping_info_t mapping_info;
            if (check_user_page(page_dir, i, &mapping_info) < 0) {
                return -1;
            }
            if (
                mapping_info != ZERO_FRAME_MAPPED &&
                mapping_info != NEW_FRAME_MAPPED
            ) {
                uint32_t availability;
                get_availability(page_dir, i, &availability);
                if (availability != PAGE_AVAILABLE) {
                    return -1;
                }
                if (zero_frame) {
                    if (map_zero_frame(page_dir, i) < 0) {
                        return -1;
                    }
                } else {
                    if (map_new_frame(page_dir, i) < 0) {
                        return -1;
                    }
                    memset((void *)(uint32_t)i, 0, PAGE_SIZE);
                }
            }
        }
    }
    return 0;
}

/*@}*/
