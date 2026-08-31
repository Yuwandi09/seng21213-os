/* =============================================================================
 * SENG21213-OS :: Round-Robin Scheduler Implementation
 * File   : kernel/scheduler.c
 * ============================================================================*/
#include "scheduler.h"
#include "process.h"
#include "vga.h"

extern void switch_to(uint32_t *prev_esp, uint32_t next_esp);

static pcb_t *current_process = NULL;
static int    current_index   = 0;
static int    quantum_left    = TIME_QUANTUM_TICKS;
static bool   scheduler_active = false;

pcb_t *process_get_current(void) {
    return current_process;
}

pcb_t *scheduler_get_current(void) {
    return current_process;
}

void scheduler_init(void) {
    process_init();

    /* Bootstrap Main Kernel / Shell as PID 0 */
    pcb_t *table = process_get_table();
    table[0].pid   = 0;
    table[0].state = PROC_RUNNING;
    table[0].ticks = 0;
    table[0].esp   = 0;
    table[0].name[0] = 's';
    table[0].name[1] = 'h';
    table[0].name[2] = 'e';
    table[0].name[3] = 'l';
    table[0].name[4] = 'l';
    table[0].name[5] = '\0';

    current_process = &table[0];
    current_index   = 0;
    quantum_left    = TIME_QUANTUM_TICKS;
    scheduler_active = true;
}

void schedule(void) {
    if (!scheduler_active || !current_process) return;

    pcb_t *table = process_get_table();
    int next_idx = -1;

    /* Scan round-robin for the next READY or RUNNING process */
    for (int i = 1; i <= MAX_PROCESSES; i++) {
        int idx = (current_index + i) % MAX_PROCESSES;
        if (table[idx].state == PROC_READY) {
            next_idx = idx;
            break;
        }
    }

    /* If no other ready process, keep running current if still alive */
    if (next_idx == -1) {
        if (current_process->state == PROC_RUNNING) {
            return;
        }
        /* If current is not running (e.g. terminated), find shell (PID 0) */
        next_idx = 0;
    }

    pcb_t *next_proc = &table[next_idx];
    if (next_proc == current_process) {
        return;
    }

    pcb_t *prev_proc = current_process;
    if (prev_proc->state == PROC_RUNNING) {
        prev_proc->state = PROC_READY;
    }

    next_proc->state = PROC_RUNNING;
    current_process  = next_proc;
    current_index    = next_idx;
    quantum_left     = TIME_QUANTUM_TICKS;

    switch_to(&prev_proc->esp, next_proc->esp);
}

void scheduler_tick(void) {
    if (!scheduler_active || !current_process) return;

    current_process->ticks++;

    if (--quantum_left <= 0) {
        quantum_left = TIME_QUANTUM_TICKS;
        schedule();
    }
}
