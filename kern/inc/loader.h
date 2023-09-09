/* The 15-410 kernel project
 *
 *     loader.h
 *
 * Structure definitions, #defines, and function prototypes
 * for the user process loader.
 */

#ifndef _LOADER_H
#define _LOADER_H

#include <ureg.h>
#include <stdint.h>

// size of guest virtual address space
#define GUEST_MEM_SIZE (0x1400000)

/* --- Prototypes --- */

int getbytes( const char *filename, int offset, int size, char *buf );

/*
 * Declare your loader prototypes here.
 */

int load_executable(char *execname, char **argvec, ureg_t *ureg_ptr);

int push_val(uint32_t *esp_ptr, void *val_ptr, uint32_t size);

#endif /* _LOADER_H */
