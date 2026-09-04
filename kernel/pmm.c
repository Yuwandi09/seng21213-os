/* =============================================================================
 * SENG21213-OS :: Physical Memory Manager Implementation
 * File   : kernel/pmm.c
 *
 * Bitmap allocator: one bit per 4 KB physical frame.
 * Bit = 0  → frame is FREE
 * Bit = 1  → frame is USED / RESERVED
 * ============================================================================*/
#include "pmm.h"
#include "vga.h"

/* ---------------------------------------------------------------------------
 * Bitmap storage
 * PMM_TOTAL_FRAMES = 8192 frames → 8192 / 32 = 256 uint32_t words
 * --------------------------------------------------------------------------*/
#define BITMAP_WORDS  (PMM_TOTAL_FRAMES / 32)

static uint32_t  pmm_bitmap[BITMAP_WORDS];
static uint32_t  used_frames = 0;

/* ---------------------------------------------------------------------------
 * Bit manipulation helpers
 * --------------------------------------------------------------------------*/
static void bitmap_set(uint32_t frame) {
    pmm_bitmap[frame / 32] |= (1u << (frame % 32));
}

static void bitmap_clear(uint32_t frame) {
    pmm_bitmap[frame / 32] &= ~(1u << (frame % 32));
}

static int bitmap_test(uint32_t frame) {
    return (pmm_bitmap[frame / 32] >> (frame % 32)) & 1u;
}

/* ---------------------------------------------------------------------------
 * pmm_init
 * --------------------------------------------------------------------------*/
void pmm_init(void) {
    /* Start with all frames marked USED for safety */
    for (int i = 0; i < BITMAP_WORDS; i++) {
        pmm_bitmap[i] = 0xFFFFFFFF;
    }
    used_frames = PMM_TOTAL_FRAMES;

    /* Mark extended memory (1 MB – 32 MB) as FREE */
    uint32_t first_free = (PMM_RESERVED_END + 1) / PMM_FRAME_SIZE; /* frame 256 */
    for (uint32_t f = first_free; f < PMM_TOTAL_FRAMES; f++) {
        bitmap_clear(f);
        used_frames--;
    }

    /* Re-mark the kernel region (0x10000 – 0x1FFFF) as USED */
    uint32_t kern_start = 0x10000 / PMM_FRAME_SIZE;
    uint32_t kern_end   = 0x1FFFF / PMM_FRAME_SIZE;
    for (uint32_t f = kern_start; f <= kern_end; f++) {
        if (!bitmap_test(f)) {
            bitmap_set(f);
            used_frames++;
        }
    }

    /* Re-mark the kernel stack region (0x80000 – 0x9FFFF) as USED */
    uint32_t stk_start = 0x80000 / PMM_FRAME_SIZE;
    uint32_t stk_end   = 0x9FFFF / PMM_FRAME_SIZE;
    for (uint32_t f = stk_start; f <= stk_end; f++) {
        if (!bitmap_test(f)) {
            bitmap_set(f);
            used_frames++;
        }
    }
}

/* ---------------------------------------------------------------------------
 * pmm_alloc_frame – find the first free frame and mark it used
 * --------------------------------------------------------------------------*/
phys_addr_t pmm_alloc_frame(void) {
    /* Scan bitmap word-by-word for efficiency */
    for (int w = 0; w < BITMAP_WORDS; w++) {
        if (pmm_bitmap[w] == 0xFFFFFFFF) continue; /* All used in this word */

        for (int b = 0; b < 32; b++) {
            uint32_t frame = (uint32_t)(w * 32 + b);
            if (!bitmap_test(frame)) {
                bitmap_set(frame);
                used_frames++;
                return (phys_addr_t)(frame * PMM_FRAME_SIZE);
            }
        }
    }
    return 0; /* Out of memory */
}

/* ---------------------------------------------------------------------------
 * pmm_free_frame – mark a frame as free
 * --------------------------------------------------------------------------*/
void pmm_free_frame(phys_addr_t addr) {
    if (addr < (PMM_RESERVED_END + 1)) return; /* Never free reserved region */

    uint32_t frame = addr / PMM_FRAME_SIZE;
    if (frame >= PMM_TOTAL_FRAMES) return;

    if (bitmap_test(frame)) {
        bitmap_clear(frame);
        if (used_frames > 0) used_frames--;
    }
}

/* ---------------------------------------------------------------------------
 * Query functions
 * --------------------------------------------------------------------------*/
uint32_t pmm_get_total_frames(void) { return PMM_TOTAL_FRAMES; }
uint32_t pmm_get_used_frames(void)  { return used_frames; }
uint32_t pmm_get_free_frames(void)  { return PMM_TOTAL_FRAMES - used_frames; }

int pmm_is_free(phys_addr_t addr) {
    uint32_t frame = addr / PMM_FRAME_SIZE;
    if (frame >= PMM_TOTAL_FRAMES) return 0;
    return !bitmap_test(frame);
}
