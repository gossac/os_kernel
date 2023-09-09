/**
 * @file segmentation.c
 * @author Tony Xi (xiaolix)
 * @author Zekun Ma (zekunm)
 * @brief initialization of segmentation
 */

#include <segmentation.h> // initialize_segmentation
#include <simics.h> // lprintf
#include <stdint.h> // uint16_t
#include <asm.h> // gdt_base
#include <seg.h> // SEGSEL_SPARE0_IDX
#include <common_kern.h> // USER_MEM_START
#include <vm.h> // VIRTUAL_ADDR_END
#include <limits.h> // CHAR_BIT
#include <interrupt.h> // USER_PL

// The scaling of the segment limit field when the granularity flag is on.
#define SEGMENT_LIMIT_SCALE (0x1000)

/**
 * @brief data structure of segemnt descriptor
 */
typedef struct segment_descriptor_t {
    uint16_t segment_limit1;
    uint16_t base_addr1;
    uint16_t base_addr2         : 8;
    uint16_t type               : 4;
    uint16_t s                  : 1;
    uint16_t dpl                : 2;
    uint16_t p                  : 1;
    uint16_t segment_limit2     : 4;
    uint16_t avl                : 1;
    uint16_t padding            : 1;
    uint16_t d_b                : 1;
    uint16_t g                  : 1;
    uint16_t base_addr3         : 8;
} segment_descriptor_t;

/**
 * @brief install segment descriptors for guests
 */
void initialize_gdt(void) {
    segment_descriptor_t *gdt = gdt_base();

    // all the guest segments range from USER_MEM_START to
    // VIRTUAL_ADDR_END - USER_MEM_START
    segment_descriptor_t user_cs_descriptor = {
        .segment_limit1 = (uint16_t)(
            (VIRTUAL_ADDR_END - USER_MEM_START - 1) /
            SEGMENT_LIMIT_SCALE
        ),
        .base_addr1 = (uint16_t)USER_MEM_START,
        .base_addr2 = (uint8_t)(
            USER_MEM_START >>(sizeof(uint16_t) * CHAR_BIT)
        ),
        .type = 10,
        .s = 1,
        .dpl = USER_PL,
        .p = 1,
        .segment_limit2 = (
            (VIRTUAL_ADDR_END - USER_MEM_START - 1) / SEGMENT_LIMIT_SCALE
        ) >> (sizeof(uint16_t) * CHAR_BIT),
        .padding = 0,
        .d_b = 1,
        .g = 1,
        .base_addr3 = (uint16_t)(
            USER_MEM_START >> (sizeof(uint16_t) * CHAR_BIT + 8)
        )
    };
    segment_descriptor_t user_ds_descriptor = user_cs_descriptor;
    user_ds_descriptor.type = 2;

    // guest kernel CS selector
    gdt[SEGSEL_SPARE0_IDX] = user_cs_descriptor;
    // guest kernel DS selector
    gdt[SEGSEL_SPARE1_IDX] = user_ds_descriptor;
    // guest user CS selector
    gdt[SEGSEL_SPARE2_IDX] = user_cs_descriptor;
    // guest user DS selector
    gdt[SEGSEL_SPARE3_IDX] = user_ds_descriptor;

    lprintf("Segmentation is initialized.");
}
