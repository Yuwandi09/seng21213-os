/* =============================================================================
 * SENG21213-OS :: Kernel Threads Implementation
 * File   : kernel/thread.c
 * ============================================================================*/
#include "thread.h"
#include "vga.h"

extern void switch_to(uint32_t *prev_esp, uint32_t next_esp);

static thread_t thread_table[MAX_THREADS];
static uint32_t next_tid     = 1;
static int      current_idx  = 0;
static thread_t *current_thread = NULL;
static bool     thread_system_active = false;

static void k_strncpy(char *dst, const char *src, size_t n) {
    size_t i = 0;
    while (i < n - 1 && src[i]) { dst[i] = src[i]; i++; }
    dst[i] = '\0';
}

thread_t *thread_get_current(void)  { return current_thread; }
thread_t *thread_get_table(void)    { return thread_table; }

void thread_init(void) {
    for (int i = 0; i < MAX_THREADS; i++) {
        thread_table[i].tid   = 0;
        thread_table[i].state = THREAD_UNUSED;
        thread_table[i].esp   = 0;
        thread_table[i].ticks = 0;
        thread_table[i].next  = NULL;
        thread_table[i].name[0] = '\0';
    }

    /* Bootstrap thread 0 = main kernel thread */
    thread_table[0].tid   = 0;
    thread_table[0].state = THREAD_RUNNING;
    thread_table[0].ticks = 0;
    k_strncpy(thread_table[0].name, "main", sizeof(thread_table[0].name));

    current_thread = &thread_table[0];
    current_idx    = 0;
    thread_system_active = true;
}

thread_t *thread_create(const char *name, void (*entry)(void)) {
    int slot = -1;
    for (int i = 1; i < MAX_THREADS; i++) {
        if (thread_table[i].state == THREAD_UNUSED ||
            thread_table[i].state == THREAD_TERMINATED) {
            slot = i;
            break;
        }
    }
    if (slot == -1) {
        vga_puts_color("  [ERROR] Thread table full!\n", VGA_LIGHT_RED, VGA_BLACK);
        return NULL;
    }

    thread_t *t = &thread_table[slot];
    t->tid   = next_tid++;
    t->state = THREAD_READY;
    t->ticks = 0;
    t->next  = NULL;
    k_strncpy(t->name, name ? name : "thread", sizeof(t->name));

    /* Build initial stack frame for switch_to: pushfd, edi, esi, ebx, ebp, ret-to-entry */
    uint32_t *stk = &t->stack[THREAD_STACK_SIZE / 4];
    *(--stk) = (uint32_t)thread_exit;  /* If entry() returns */
    *(--stk) = (uint32_t)entry;        /* EIP (switch_to ret lands here) */
    *(--stk) = 0;                      /* EBP */
    *(--stk) = 0;                      /* EBX */
    *(--stk) = 0;                      /* ESI */
    *(--stk) = 0;                      /* EDI */
    *(--stk) = 0x00000202;             /* EFLAGS: popped first by popfd */
    t->esp = (uint32_t)stk;

    return t;
}

void thread_schedule(void) {
    if (!thread_system_active || !current_thread) return;

    int next_idx = -1;
    for (int i = 1; i <= MAX_THREADS; i++) {
        int idx = (current_idx + i) % MAX_THREADS;
        if (thread_table[idx].state == THREAD_READY) {
            next_idx = idx;
            break;
        }
    }

    if (next_idx == -1) {
        /* No other ready thread — keep running current if still alive */
        if (current_thread->state == THREAD_RUNNING) return;
        next_idx = 0; /* Fall back to main thread */
    }

    thread_t *next = &thread_table[next_idx];
    if (next == current_thread) return;

    thread_t *prev = current_thread;
    if (prev->state == THREAD_RUNNING) prev->state = THREAD_READY;

    next->state    = THREAD_RUNNING;
    current_thread = next;
    current_idx    = next_idx;

    switch_to(&prev->esp, next->esp);
}

void thread_tick(void) {
    if (!thread_system_active || !current_thread) return;
    current_thread->ticks++;
}

void thread_yield(void) {
    thread_schedule();
}

void thread_exit(void) {
    if (current_thread && current_thread->tid != 0) {
        current_thread->state = THREAD_TERMINATED;
    }
    thread_schedule();
    while (1) { __asm__ __volatile__("hlt"); }
}

int thread_kill(uint32_t tid) {
    if (tid == 0) {
        vga_puts_color("  Cannot kill main thread (TID 0).\n", VGA_LIGHT_RED, VGA_BLACK);
        return -1;
    }
    for (int i = 0; i < MAX_THREADS; i++) {
        if (thread_table[i].tid == tid &&
            thread_table[i].state != THREAD_UNUSED &&
            thread_table[i].state != THREAD_TERMINATED) {
            thread_table[i].state = THREAD_TERMINATED;
            vga_printf("  Thread %d (%s) killed.\n", tid, thread_table[i].name);
            return 0;
        }
    }
    vga_printf("  Thread TID %d not found.\n", tid);
    return -1;
}
