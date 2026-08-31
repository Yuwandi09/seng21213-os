/* =============================================================================
 * SENG21213-OS :: Kernel Threads Header
 * File   : kernel/thread.h
 * Stage 2: Kernel-level threads sharing the process address space.
 *          Each thread has its own stack but shares the kernel's memory.
 * ============================================================================*/
#ifndef THREAD_H
#define THREAD_H

#include "../include/types.h"

#define MAX_THREADS  16
#define THREAD_STACK_SIZE 4096

typedef enum {
    THREAD_UNUSED = 0,
    THREAD_READY,
    THREAD_RUNNING,
    THREAD_BLOCKED,
    THREAD_TERMINATED
} thread_state_t;

typedef struct thread {
    uint32_t       tid;
    char           name[32];
    thread_state_t state;
    uint32_t       esp;
    uint32_t       stack[THREAD_STACK_SIZE / 4];
    uint32_t       ticks;
    struct thread *next;
} thread_t;

void      thread_init(void);
thread_t *thread_create(const char *name, void (*entry)(void));
void      thread_yield(void);
void      thread_exit(void);
int       thread_kill(uint32_t tid);
thread_t *thread_get_current(void);
thread_t *thread_get_table(void);
void      thread_schedule(void);
void      thread_tick(void);

#endif /* THREAD_H */
