/* =============================================================================
 * SENG21213-OS :: Counting Semaphore Implementation
 * File   : kernel/semaphore.c
 * ============================================================================*/
#include "semaphore.h"
#include "thread.h"

static void k_strncpy(char *dst, const char *src, size_t n) {
    size_t i = 0;
    while (i < n - 1 && src[i]) { dst[i] = src[i]; i++; }
    dst[i] = '\0';
}

void sem_init(semaphore_t *s, int initial, const char *name) {
    s->count = initial;
    k_strncpy(s->name, name ? name : "sem", sizeof(s->name));
}

void sem_wait(semaphore_t *s) {
    /* Spin-wait with yield until count > 0, then atomically decrement */
    while (1) {
        __asm__ __volatile__("cli");
        if (s->count > 0) {
            s->count--;
            __asm__ __volatile__("sti");
            return;
        }
        __asm__ __volatile__("sti");
        thread_yield(); /* Yield CPU while blocked */
    }
}

void sem_signal(semaphore_t *s) {
    __asm__ __volatile__("cli");
    s->count++;
    __asm__ __volatile__("sti");
}

int sem_trywait(semaphore_t *s) {
    __asm__ __volatile__("cli");
    if (s->count > 0) {
        s->count--;
        __asm__ __volatile__("sti");
        return 1;
    }
    __asm__ __volatile__("sti");
    return 0;
}
