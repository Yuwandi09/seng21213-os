/* =============================================================================
 * SENG21213-OS :: Process Management Implementation
 * File   : kernel/process.c
 * ============================================================================*/
#include "process.h"
#include "scheduler.h"
#include "vga.h"

static pcb_t pcb_table[MAX_PROCESSES];
static uint32_t next_pid = 1;

static void k_strncpy(char *dst, const char *src, size_t n) {
    size_t i = 0;
    while (i < n - 1 && src[i]) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

void process_init(void) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        pcb_table[i].pid   = 0;
        pcb_table[i].state = PROC_UNUSED;
        pcb_table[i].esp   = 0;
        pcb_table[i].ticks = 0;
        pcb_table[i].next  = NULL;
        pcb_table[i].name[0] = '\0';
    }
}

pcb_t *process_get_table(void) {
    return pcb_table;
}

pcb_t *process_create(const char *name, void (*entry)(void)) {
    int slot = -1;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (pcb_table[i].state == PROC_UNUSED || pcb_table[i].state == PROC_TERMINATED) {
            slot = i;
            break;
        }
    }

    if (slot == -1) {
        vga_puts_color("  [ERROR] Process table full! Cannot create process.\n", VGA_LIGHT_RED, VGA_BLACK);
        return NULL;
    }

    pcb_t *p = &pcb_table[slot];
    p->pid   = next_pid++;
    p->state = PROC_READY;
    p->ticks = 0;
    p->next  = NULL;
    k_strncpy(p->name, name ? name : "process", sizeof(p->name));

    /* Initialize stack frame so switch_to pops registers and returns to entry() */
    uint32_t *stk = &p->stack[STACK_SIZE / 4];
    *(--stk) = (uint32_t)process_exit; /* Return address if entry() exits */
    *(--stk) = (uint32_t)entry;        /* Initial EIP for switch_to ret */
    *(--stk) = 0x00000202;             /* EFLAGS: Interrupt Flag (IF) enabled */
    *(--stk) = 0;                      /* EDI */
    *(--stk) = 0;                      /* ESI */
    *(--stk) = 0;                      /* EBX */
    *(--stk) = 0;                      /* EBP */

    p->esp = (uint32_t)stk;
    return p;
}

void process_yield(void) {
    schedule();
}

void process_exit(void) {
    pcb_t *curr = process_get_current();
    if (curr && curr->pid != 0) {
        curr->state = PROC_TERMINATED;
    }
    schedule();
    /* In case we get returned to terminated process */
    while (1) { __asm__ __volatile__("hlt"); }
}

int process_kill(uint32_t pid) {
    if (pid == 0) {
        vga_puts_color("  Cannot kill kernel shell (PID 0).\n", VGA_LIGHT_RED, VGA_BLACK);
        return -1;
    }

    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (pcb_table[i].pid == pid && pcb_table[i].state != PROC_UNUSED && pcb_table[i].state != PROC_TERMINATED) {
            pcb_table[i].state = PROC_TERMINATED;
            vga_printf("  Process %d (%s) terminated.\n", pid, pcb_table[i].name);
            return 0;
        }
    }
    vga_printf("  Process PID %d not found.\n", pid);
    return -1;
}
