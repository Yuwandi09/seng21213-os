/* =============================================================================
 * SENG21213-OS :: Physical Memory Manager (PMM) Header
 * File   : kernel/pmm.h
 * Stage 3: Bitmap-based physical page frame allocator.
 *
 * Memory layout (QEMU -m 32M):
 *   0x00000000 – 0x000FFFFF  : First 1 MB (BIOS, VGA, boot – reserved)
 *   0x00010000 – 0x0001FFFF  : Kernel image (~64 KB loaded here)
 *   0x00090000               : Kernel stack (64 KB at 0x90000)
 *   0x00100000 – 0x01FFFFFF  : Extended memory (~31 MB – usable)
 *
 * Each bit in the bitmap represents one 4 KB physical frame.
 *   0 = FREE, 1 = USED
 * ============================================================================*/
#ifndef PMM_H
#define PMM_H

#include "../include/types.h"

#define PMM_FRAME_SIZE      4096            /* 4 KB per frame             */
#define PMM_TOTAL_RAM       (32 * 1024 * 1024)  /* 32 MB (matches -m 32M) */
#define PMM_TOTAL_FRAMES    (PMM_TOTAL_RAM / PMM_FRAME_SIZE)  /* 8192    */

/* Addresses reserved from allocation */
#define PMM_RESERVED_START  0x00000000
#define PMM_RESERVED_END    0x000FFFFF  /* First 1 MB always reserved */

/* Physical address type */
typedef uint32_t phys_addr_t;

void        pmm_init(void);

/* Allocate one free 4 KB frame. Returns physical address, or 0 on failure. */
phys_addr_t pmm_alloc_frame(void);

/* Free a previously allocated frame by physical address. */
void        pmm_free_frame(phys_addr_t addr);

/* Query functions */
uint32_t    pmm_get_total_frames(void);
uint32_t    pmm_get_used_frames(void);
uint32_t    pmm_get_free_frames(void);

/* Check if a physical address is free */
int         pmm_is_free(phys_addr_t addr);

#endif /* PMM_H */
