/**
 * @file segmentation.h
 * @author Tony Xi (xiaolix)
 * @author Zekun Ma (zekunm)
 * @brief segmentation related macros and functions
 */

#ifndef SEGMENTATION_H_SEEN
#define SEGMENTATION_H_SEEN

#include <interrupt.h> // USER_PL
#include <seg.h> // SEGSEL_SPARE0

// to correct the RPL field of each partial segment selector
#define SEGSEL_GUEST_RPL_MASK (USER_PL)

// segment selectors for guests
#define SEGSEL_GUEST_KERNEL_CS (SEGSEL_SPARE0 | SEGSEL_GUEST_RPL_MASK)
#define SEGSEL_GUEST_KERNEL_DS (SEGSEL_SPARE1 | SEGSEL_GUEST_RPL_MASK)
#define SEGSEL_GUEST_USER_CS (SEGSEL_SPARE2 | SEGSEL_GUEST_RPL_MASK)
#define SEGSEL_GUEST_USER_DS (SEGSEL_SPARE3 | SEGSEL_GUEST_RPL_MASK)

void initialize_gdt(void);

#endif // SEGMENTATION_H_SEEN
