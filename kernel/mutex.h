/* =============================================================================
 * SENG21213-OS :: Mutex (Mutual Exclusion Lock) Header
 * File   : kernel/mutex.h
 * Stage 2: Spinlock-based mutex for critical section protection.
 * ============================================================================*/
#ifndef MUTEX_H
#define MUTEX_H

#include "../include/types.h"

typedef struct {
    volatile int locked;    /* 0 = unlocked, 1 = locked */
    char         name[16];
} mutex_t;

void mutex_init(mutex_t *m, const char *name);
void mutex_lock(mutex_t *m);
void mutex_unlock(mutex_t *m);
int  mutex_trylock(mutex_t *m);

#endif /* MUTEX_H */
