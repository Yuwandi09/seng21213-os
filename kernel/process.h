/* =============================================================================
 * SENG21213-OS :: Process Control Block (PCB) & Management Header
 * File   : kernel/process.h
 * ============================================================================*/
#ifndef PROCESS_H
#define PROCESS_H

#include "../include/types.h"

#define MAX_PROCESSES 16
#define STACK_SIZE    4096

typedef enum {
    PROC_UNUSED = 0,
    PROC_READY,
    PROC_RUNNING,
    PROC_BLOCKED,
    PROC_TERMINATED
} proc_state_t;

typedef struct pcb {
    uint32_t     pid;
    char         name[32];
    proc_state_t state;
    uint32_t     esp;                  /* Saved stack pointer */
    uint32_t     stack[STACK_SIZE / 4];
    uint32_t     ticks;                /* CPU execution ticks */
    struct pcb  *next;
} pcb_t;

void   process_init(void);
pcb_t *process_create(const char *name, void (*entry)(void));
void   process_yield(void);
void   process_exit(void);
int    process_kill(uint32_t pid);
pcb_t *process_get_current(void);
pcb_t *process_get_table(void);

#endif /* PROCESS_H */
