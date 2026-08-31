/* =============================================================================
 * SENG21213-OS :: Round-Robin Scheduler Header
 * File   : kernel/scheduler.h
 * ============================================================================*/
#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "../include/types.h"
#include "process.h"

#define TIME_QUANTUM_TICKS 2 /* 20 ms time slice per process */

void   scheduler_init(void);
void   scheduler_tick(void);
void   schedule(void);
pcb_t *scheduler_get_current(void);

#endif /* SCHEDULER_H */
