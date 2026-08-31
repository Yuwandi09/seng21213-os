/* =============================================================================
 * SENG21213-OS :: Mutex Implementation
 * File   : kernel/mutex.c
 * ============================================================================*/
#include "mutex.h"
#include "thread.h"

static void k_strncpy(char *dst, const char *src, size_t n) {
    size_t i = 0;
    while (i < n - 1 && src[i]) { dst[i] = src[i]; i++; }
    dst[i] = '\0';
}

void mutex_init(mutex_t *m, const char *name) {
    m->locked = 0;
    k_strncpy(m->name, name ? name : "mutex", sizeof(m->name));
}

/* Atomic test-and-set using XCHG instruction (implicit LOCK prefix) */
static int xchg(volatile int *ptr, int newval) {
    int result;
    __asm__ __volatile__(
        "xchgl %0, %1"
        : "=r"(result), "+m"(*ptr)
        : "0"(newval)
        : "memory"
    );
    return result;
}

void mutex_lock(mutex_t *m) {
    /* Spin until we acquire the lock, yielding each iteration */
    while (xchg(&m->locked, 1) != 0) {
        thread_yield(); /* Give up CPU while waiting */
    }
}

void mutex_unlock(mutex_t *m) {
    __asm__ __volatile__("" ::: "memory"); /* Compiler barrier */
    m->locked = 0;
}

int mutex_trylock(mutex_t *m) {
    return xchg(&m->locked, 1) == 0 ? 1 : 0;
}
