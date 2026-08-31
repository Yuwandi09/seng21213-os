/* =============================================================================
 * SENG21213-OS :: Counting Semaphore Header
 * File   : kernel/semaphore.h
 * Stage 2: Classic counting semaphore (Dijkstra P/V operations).
 * ============================================================================*/
#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include "../include/types.h"

typedef struct {
    volatile int count;
    char         name[16];
} semaphore_t;

void sem_init(semaphore_t *s, int initial, const char *name);
void sem_wait(semaphore_t *s);   /* P() / Down() – decrement, block if 0 */
void sem_signal(semaphore_t *s); /* V() / Up()   – increment, wake waiter */
int  sem_trywait(semaphore_t *s);

#endif /* SEMAPHORE_H */
