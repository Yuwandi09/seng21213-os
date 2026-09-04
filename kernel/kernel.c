/* =============================================================================
 * SENG21213-OS :: Main Kernel (Stage 4 – In-Memory RAM-Disk File System)
 * File   : kernel/kernel.c
 * ============================================================================*/

#include "vga.h"
#include "keyboard.h"
#include "idt.h"
#include "pit.h"
#include "process.h"
#include "scheduler.h"
#include "thread.h"
#include "mutex.h"
#include "semaphore.h"
#include "pmm.h"
#include "fs.h"
#include "../include/types.h"

static void cmd_help(void);
static void cmd_clear(void);
static void cmd_about(void);
static void cmd_version(void);
static void cmd_echo(const char *args);
static void cmd_colour(const char *args);
static void cmd_halt(void);
static void cmd_mem(void);
static void cmd_meminfo(void);
static void cmd_ps(void);
static void cmd_kill(const char *args);
static void cmd_threads(void);
static void cmd_demo(void);
static void cmd_mutex_demo(void);
static void cmd_prodcon(void);
static void cmd_ls(void);
static void cmd_touch(const char *args);
static void cmd_write(const char *args);
static void cmd_cat(const char *args);
static void cmd_rm(const char *args);

/* ---------------------------------------------------------------------------
 * String utilities
 * --------------------------------------------------------------------------*/
static int k_strcmp(const char *a, const char *b) {
    while (*a && (*a == *b)) { a++; b++; }
    return (uint8_t)*a - (uint8_t)*b;
}

static int k_strncmp(const char *a, const char *b, size_t n) {
    while (n-- && *a && (*a == *b)) { a++; b++; }
    return n == (size_t)-1 ? 0 : (uint8_t)*a - (uint8_t)*b;
}

static size_t k_strlen(const char *s) {
    size_t n = 0;
    while (s[n]) n++;
    return n;
}

static const char *k_ltrim(const char *s) {
    while (*s == ' ') s++;
    return s;
}

static int k_atoi(const char **str) {
    int val = 0;
    const char *s = *str;
    while (*s == ' ') s++;
    while (*s >= '0' && *s <= '9') {
        val = val * 10 + (*s - '0');
        s++;
    }
    *str = s;
    return val;
}

/* ---------------------------------------------------------------------------
 * Progress bar helper
 * --------------------------------------------------------------------------*/
static void draw_bar(uint32_t used, uint32_t total, int width, vga_color_t col) {
    int filled = (int)((used * (uint32_t)width) / (total ? total : 1));
    vga_putchar('[');
    for (int i = 0; i < width; i++) {
        if (i < filled) {
            vga_puts_color("|", col, VGA_BLACK);
        } else {
            vga_putchar('-');
        }
    }
    vga_putchar(']');
}

/* ---------------------------------------------------------------------------
 * Splash screen
 * --------------------------------------------------------------------------*/
static void print_splash(void) {
    vga_clear(VGA_BLACK);
    vga_draw_box(0, 0, 7, 80, VGA_LIGHT_MAGENTA);

    vga_set_cursor(1, 2);
    vga_puts_color("  SENG21213-OS  |  Computer Architecture & Operating Systems",
                   VGA_YELLOW, VGA_BLACK);
    vga_set_cursor(2, 2);
    vga_puts_color("  Stage 4: RAM-Disk File System & Complete Kernel (v0.5-stage4)",
                   VGA_LIGHT_CYAN, VGA_BLACK);
    vga_set_cursor(3, 2);
    vga_puts_color("  Faculty of Engineering – Department of Software Engineering",
                   VGA_LIGHT_GREY, VGA_BLACK);
    vga_set_cursor(4, 2);
    vga_puts_color("  Commands: help | ls | touch | write | cat | rm | meminfo | ps | threads",
                   VGA_LIGHT_GREEN, VGA_BLACK);
    vga_set_cursor(5, 2);
    vga_puts_color("  FS: In-Memory RAM Disk | 16 file slots | 1024 bytes/file",
                   VGA_DARK_GREY, VGA_BLACK);

    vga_set_cursor(8, 0);
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    vga_puts("  [  OK  ] IDT, 8259 PIC, i8253 PIT @ 100 Hz (IRQ0)\n");
    vga_puts("  [  OK  ] Round-Robin process scheduler active\n");
    vga_puts("  [  OK  ] Kernel threads + mutex + semaphore ready\n");

    uint32_t free_f = pmm_get_free_frames();
    uint32_t total_f = pmm_get_total_frames();
    vga_printf("  [  OK  ] PMM initialized: %d KB free / %d KB total\n",
               free_f * 4, total_f * 4);
    vga_printf("  [  OK  ] In-memory RAM disk ready (%d files present)\n\n", fs_file_count());
}

/* ---------------------------------------------------------------------------
 * Stage 1 process workers
 * --------------------------------------------------------------------------*/
static void worker1_task(void) {
    for (int i = 0; i < 8; i++) {
        vga_puts_color(" [P1] ", VGA_LIGHT_CYAN, VGA_BLACK);
        pit_sleep(25);
    }
    vga_puts_color(" [P1 Done] ", VGA_LIGHT_GREEN, VGA_BLACK);
    process_exit();
}

static void worker2_task(void) {
    for (int i = 0; i < 8; i++) {
        vga_puts_color(" [P2] ", VGA_YELLOW, VGA_BLACK);
        pit_sleep(35);
    }
    vga_puts_color(" [P2 Done] ", VGA_LIGHT_GREEN, VGA_BLACK);
    process_exit();
}

/* ---------------------------------------------------------------------------
 * Stage 2 sync demos
 * --------------------------------------------------------------------------*/
static volatile int shared_counter = 0;
static mutex_t      counter_mutex;

static void counter_thread_safe(void) {
    for (int i = 0; i < 5; i++) {
        mutex_lock(&counter_mutex);
        int tmp = shared_counter;
        pit_sleep(1);
        shared_counter = tmp + 1;
        mutex_unlock(&counter_mutex);
        thread_yield();
    }
    vga_puts_color(" [T:done] ", VGA_LIGHT_GREEN, VGA_BLACK);
    thread_exit();
}

#define BUFFER_SIZE 4
static volatile int  pc_buffer[BUFFER_SIZE];
static volatile int  pc_head = 0;
static volatile int  pc_tail = 0;
static semaphore_t   sem_empty;
static semaphore_t   sem_full;
static mutex_t       pc_mutex;

static void producer_thread(void) {
    for (int i = 1; i <= 6; i++) {
        sem_wait(&sem_empty);
        mutex_lock(&pc_mutex);
        pc_buffer[pc_head] = i;
        pc_head = (pc_head + 1) % BUFFER_SIZE;
        vga_printf(" [PROD:%d] ", i);
        mutex_unlock(&pc_mutex);
        sem_signal(&sem_full);
        pit_sleep(15);
    }
    vga_puts_color(" [Producer Done] ", VGA_LIGHT_GREEN, VGA_BLACK);
    thread_exit();
}

static void consumer_thread(void) {
    for (int i = 0; i < 6; i++) {
        sem_wait(&sem_full);
        mutex_lock(&pc_mutex);
        int item = pc_buffer[pc_tail];
        pc_tail = (pc_tail + 1) % BUFFER_SIZE;
        vga_printf(" [CONS:%d] ", item);
        mutex_unlock(&pc_mutex);
        sem_signal(&sem_empty);
        pit_sleep(20);
    }
    vga_puts_color(" [Consumer Done] ", VGA_LIGHT_GREEN, VGA_BLACK);
    thread_exit();
}

/* ---------------------------------------------------------------------------
 * File System Shell Commands
 * --------------------------------------------------------------------------*/
static void cmd_ls(void) {
    fs_file_t *files = fs_get_files();
    int count = 0;

    vga_puts_color("\n  NAME                 SIZE (bytes)\n", VGA_LIGHT_CYAN, VGA_BLACK);
    vga_puts("  ───────────────────────────────────\n");

    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (files[i].used) {
            count++;
            vga_printf("  %-20s %d B\n", files[i].name, files[i].size);
        }
    }

    if (count == 0) {
        vga_puts("  (no files found)\n");
    }

    vga_printf("\n  Total: %d file(s)  [Slots free: %d / %d]\n\n",
               count, FS_MAX_FILES - count, FS_MAX_FILES);
}

static void cmd_touch(const char *args) {
    const char *name = k_ltrim(args);
    if (!*name) {
        vga_puts_color("  Usage: touch <filename>\n", VGA_YELLOW, VGA_BLACK);
        return;
    }

    int rc = fs_create(name);
    if (rc == 0) {
        vga_printf("  Created file: %s\n", name);
    } else if (rc == -2) {
        vga_printf("  File '%s' already exists.\n", name);
    } else {
        vga_puts_color("  Error: Disk full!\n", VGA_LIGHT_RED, VGA_BLACK);
    }
}

static void cmd_write(const char *args) {
    const char *p = k_ltrim(args);
    if (!*p) {
        vga_puts_color("  Usage: write <filename> <text>\n", VGA_YELLOW, VGA_BLACK);
        return;
    }

    char name[FS_MAX_NAME_LEN];
    size_t i = 0;
    while (*p && *p != ' ' && i < sizeof(name) - 1) {
        name[i++] = *p++;
    }
    name[i] = '\0';

    p = k_ltrim(p);
    if (!*p) {
        vga_puts_color("  Usage: write <filename> <text>\n", VGA_YELLOW, VGA_BLACK);
        return;
    }

    int bytes = fs_write(name, p, k_strlen(p));
    if (bytes >= 0) {
        vga_printf("  Wrote %d bytes to '%s'.\n", bytes, name);
    } else {
        vga_printf("  Error: File '%s' not found. Use 'touch %s' first.\n", name, name);
    }
}

static void cmd_cat(const char *args) {
    const char *name = k_ltrim(args);
    if (!*name) {
        vga_puts_color("  Usage: cat <filename>\n", VGA_YELLOW, VGA_BLACK);
        return;
    }

    static char read_buf[FS_MAX_FILE_SIZE];
    int bytes = fs_read(name, read_buf, sizeof(read_buf));
    if (bytes >= 0) {
        vga_puts_color("\n--- ", VGA_DARK_GREY, VGA_BLACK);
        vga_puts_color(name, VGA_LIGHT_CYAN, VGA_BLACK);
        vga_puts_color(" ---\n", VGA_DARK_GREY, VGA_BLACK);
        vga_puts(read_buf);
        vga_puts("\n");
    } else {
        vga_printf("  File '%s' not found.\n", name);
    }
}

static void cmd_rm(const char *args) {
    const char *name = k_ltrim(args);
    if (!*name) {
        vga_puts_color("  Usage: rm <filename>\n", VGA_YELLOW, VGA_BLACK);
        return;
    }

    int rc = fs_delete(name);
    if (rc == 0) {
        vga_printf("  Deleted '%s'.\n", name);
    } else {
        vga_printf("  File '%s' not found.\n", name);
    }
}

/* ---------------------------------------------------------------------------
 * Shell command implementations
 * --------------------------------------------------------------------------*/
static void cmd_help(void) {
    vga_puts_color("\n  SENG21213-OS Shell Commands (Stage 4 – Complete)\n", VGA_YELLOW, VGA_BLACK);
    vga_puts("  ──────────────────────────────────────────────────────────────\n");
    vga_puts("  help             – Show this help message\n");
    vga_puts("  clear            – Clear the screen\n");
    vga_puts("  echo <text>      – Echo text to screen\n");
    vga_puts("  version          – Print OS kernel version\n");
    vga_puts("  colour <fg> <bg> – Set text colours (0-15)\n");
    vga_puts("  halt             – Halt CPU\n");
    vga_puts("  about            – About this OS\n");
    vga_puts("  mem              – Physical memory map\n");
    vga_puts_color("\n  Stage 1 – Processes:\n", VGA_LIGHT_CYAN, VGA_BLACK);
    vga_puts("  ps               – List all processes\n");
    vga_puts("  kill <pid>       – Terminate a process\n");
    vga_puts("  demo             – Run concurrent process demo\n");
    vga_puts_color("\n  Stage 2 – Threads & Sync:\n", VGA_LIGHT_CYAN, VGA_BLACK);
    vga_puts("  threads          – List all kernel threads\n");
    vga_puts("  mutex-demo       – Shared counter with mutex protection\n");
    vga_puts("  prodcon          – Producer-Consumer semaphore demo\n");
    vga_puts_color("\n  Stage 3 – Physical Memory Manager:\n", VGA_LIGHT_CYAN, VGA_BLACK);
    vga_puts("  meminfo          – Show PMM stats + allocate/free demo\n");
    vga_puts_color("\n  Stage 4 – RAM-Disk File System:\n", VGA_LIGHT_CYAN, VGA_BLACK);
    vga_puts("  ls               – List all files in RAM disk\n");
    vga_puts("  touch <file>     – Create a new empty file\n");
    vga_puts("  write <file> <s> – Write string data to file\n");
    vga_puts("  cat <file>       – Display file contents\n");
    vga_puts("  rm <file>        – Remove file from RAM disk\n\n");
}

static void cmd_clear(void)  { vga_clear(VGA_BLACK); }

static void cmd_about(void) {
    vga_puts_color("\n  About SENG21213-OS (Complete Kernel)\n", VGA_LIGHT_CYAN, VGA_BLACK);
    vga_puts("  ─────────────────────────────────────────────\n");
    vga_puts("  Architecture : x86 (i686), 32-bit Protected Mode\n");
    vga_puts("  Scheduler    : Round-Robin, 100 Hz PIT (IRQ0)\n");
    vga_puts("  Threads      : 16-slot kernel thread table\n");
    vga_puts("  Sync         : mutex_t (XCHG spinlock), semaphore_t\n");
    vga_puts("  PMM          : Bitmap allocator, 4 KB frames, 32 MB RAM\n");
    vga_puts("  File System  : Flat in-memory RAM disk (16 files, 1 KB each)\n\n");
}

static void cmd_version(void) {
    vga_puts_color("\n  SENG21213-OS v0.5-stage4\n", VGA_LIGHT_CYAN, VGA_BLACK);
    vga_puts("  Stage 4: In-Memory RAM-Disk File System\n");
    vga_puts("  Full 5-stage kernel implementation complete!\n\n");
}

static void cmd_echo(const char *args) {
    vga_puts("  "); vga_puts(args); vga_puts("\n");
}

static void cmd_colour(const char *args) {
    const char *p = k_ltrim(args);
    if (!*p) {
        vga_puts_color("  Usage: colour <fg 0-15> <bg 0-15>\n", VGA_YELLOW, VGA_BLACK);
        return;
    }
    int fg = k_atoi(&p);
    p = k_ltrim(p);
    int bg = *p ? k_atoi(&p) : 0;
    if (fg < 0 || fg > 15 || bg < 0 || bg > 15) {
        vga_puts_color("  Colours must be 0-15.\n", VGA_LIGHT_RED, VGA_BLACK);
        return;
    }
    vga_set_color((vga_color_t)fg, (vga_color_t)bg);
    vga_puts("  Colour updated.\n");
}

static void cmd_halt(void) {
    vga_puts_color("\n  System halted.\n", VGA_LIGHT_RED, VGA_BLACK);
    __asm__ __volatile__("cli; hlt");
    while (1) { __asm__ __volatile__("hlt"); }
}

static void cmd_mem(void) {
    vga_puts_color("\n  Physical Memory Map (x86 i686, 32 MB)\n", VGA_LIGHT_CYAN, VGA_BLACK);
    vga_puts("  ──────────────────────────────────────────────────────\n");
    vga_puts("  0x00000000 – 0x000004FF  :  Real Mode IVT + BDA\n");
    vga_puts("  0x00000500 – 0x00007BFF  :  Conventional RAM (free)\n");
    vga_puts("  0x00007C00 – 0x00007DFF  :  MBR Bootloader\n");
    vga_puts("  0x00010000 – 0x0001FFFF  :  Kernel image\n");
    vga_puts("  0x00080000 – 0x0009FFFF  :  Kernel stack region\n");
    vga_puts("  0x000A0000 – 0x000BFFFF  :  Video RAM (VGA 0xB8000)\n");
    vga_puts("  0x000C0000 – 0x000FFFFF  :  BIOS ROM / reserved\n");
    vga_puts("  0x00100000 – 0x01FFFFFF  :  Extended memory (~31 MB, usable)\n\n");
}

static void cmd_meminfo(void) {
    uint32_t total_f = pmm_get_total_frames();
    uint32_t used_f  = pmm_get_used_frames();
    uint32_t free_f  = pmm_get_free_frames();

    vga_puts_color("\n  Physical Memory Manager — Frame Statistics\n", VGA_LIGHT_CYAN, VGA_BLACK);
    vga_puts("  ──────────────────────────────────────────────────────\n");
    vga_printf("  Frame size  : %d bytes (4 KB)\n", PMM_FRAME_SIZE);
    vga_printf("  Total frames: %d  (%d MB)\n", total_f, (total_f * 4) / 1024);
    vga_printf("  Used frames : %d  (%d KB)\n", used_f,  used_f  * 4);
    vga_printf("  Free frames : %d  (%d KB)\n", free_f,  free_f  * 4);

    vga_puts("\n  Usage: ");
    draw_bar(used_f, total_f, 40, VGA_LIGHT_RED);
    vga_printf(" %d%%\n", (used_f * 100) / (total_f ? total_f : 1));

    vga_puts_color("\n  [Demo] Allocating 4 test frames...\n", VGA_YELLOW, VGA_BLACK);
    phys_addr_t frames[4];
    for (int i = 0; i < 4; i++) {
        frames[i] = pmm_alloc_frame();
        if (frames[i]) {
            vga_printf("    Allocated frame at physical 0x%x\n", frames[i]);
        }
    }

    vga_puts_color("\n  [Demo] Freeing test frames...\n", VGA_LIGHT_GREEN, VGA_BLACK);
    for (int i = 0; i < 4; i++) {
        if (frames[i]) {
            pmm_free_frame(frames[i]);
            vga_printf("    Freed frame at physical 0x%x\n", frames[i]);
        }
    }

    vga_printf("\n  Free frames after demo: %d (%d KB)\n\n",
               pmm_get_free_frames(), pmm_get_free_frames() * 4);
}

static const char *proc_state_str(proc_state_t s) {
    switch (s) {
        case PROC_READY:      return "READY";
        case PROC_RUNNING:    return "RUNNING";
        case PROC_BLOCKED:    return "BLOCKED";
        case PROC_TERMINATED: return "TERMINATED";
        default:              return "UNUSED";
    }
}

static void cmd_ps(void) {
    pcb_t *table = process_get_table();
    vga_puts_color("\n  PID   STATE        TICKS   NAME\n", VGA_LIGHT_CYAN, VGA_BLACK);
    vga_puts("  ──────────────────────────────────────────\n");
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (table[i].state != PROC_UNUSED) {
            const char *st = proc_state_str(table[i].state);
            vga_printf("  %d     ", table[i].pid);
            vga_puts(st);
            int sl = k_strlen(st);
            for (int s = 0; s < 13 - sl; s++) vga_putchar(' ');
            vga_printf("%d", table[i].ticks);
            int tl = 1; uint32_t t = table[i].ticks;
            while (t >= 10) { tl++; t /= 10; }
            for (int s = 0; s < 8 - tl; s++) vga_putchar(' ');
            vga_printf("%s\n", table[i].name);
        }
    }
    vga_puts("\n");
}

static void cmd_kill(const char *args) {
    const char *p = k_ltrim(args);
    if (!*p) { vga_puts_color("  Usage: kill <pid>\n", VGA_YELLOW, VGA_BLACK); return; }
    int pid = k_atoi(&p);
    process_kill((uint32_t)pid);
}

static const char *thr_state_str(thread_state_t s) {
    switch (s) {
        case THREAD_READY:      return "READY";
        case THREAD_RUNNING:    return "RUNNING";
        case THREAD_BLOCKED:    return "BLOCKED";
        case THREAD_TERMINATED: return "TERMINATED";
        default:                return "UNUSED";
    }
}

static void cmd_threads(void) {
    thread_t *table = thread_get_table();
    vga_puts_color("\n  TID   STATE        TICKS   NAME\n", VGA_LIGHT_CYAN, VGA_BLACK);
    vga_puts("  ──────────────────────────────────────────\n");
    for (int i = 0; i < MAX_THREADS; i++) {
        if (table[i].state != THREAD_UNUSED) {
            const char *st = thr_state_str(table[i].state);
            vga_printf("  %d     ", table[i].tid);
            vga_puts(st);
            int sl = k_strlen(st);
            for (int s = 0; s < 13 - sl; s++) vga_putchar(' ');
            vga_printf("%d", table[i].ticks);
            int tl = 1; uint32_t t = table[i].ticks;
            while (t >= 10) { tl++; t /= 10; }
            for (int s = 0; s < 8 - tl; s++) vga_putchar(' ');
            vga_printf("%s\n", table[i].name);
        }
    }
    vga_puts("\n");
}

static void cmd_demo(void) {
    vga_puts_color("\n  [Stage 1 Demo] Spawning 2 concurrent processes...\n", VGA_YELLOW, VGA_BLACK);
    process_create("p-worker1", worker1_task);
    process_create("p-worker2", worker2_task);
}

static void cmd_mutex_demo(void) {
    vga_puts_color("\n  [Stage 2 – Mutex Demo] Two threads increment shared counter (expect 10)\n",
                   VGA_LIGHT_CYAN, VGA_BLACK);
    shared_counter = 0;
    mutex_init(&counter_mutex, "counter");
    thread_create("safe-t1", counter_thread_safe);
    thread_create("safe-t2", counter_thread_safe);
    pit_sleep(120);
    vga_printf("\n  Final counter = %d (expected 10)\n\n", shared_counter);
}

static void cmd_prodcon(void) {
    vga_puts_color("\n  [Stage 2 – Producer-Consumer] Semaphore + Mutex on 4-slot buffer\n",
                   VGA_LIGHT_CYAN, VGA_BLACK);
    pc_head = 0; pc_tail = 0;
    sem_init(&sem_empty, BUFFER_SIZE, "empty");
    sem_init(&sem_full,  0,           "full");
    mutex_init(&pc_mutex, "pc-lock");
    thread_create("producer", producer_thread);
    thread_create("consumer", consumer_thread);
    vga_puts("\n  Threads spawned. Run 'threads' to observe state.\n\n");
}

/* ---------------------------------------------------------------------------
 * Shell loop
 * --------------------------------------------------------------------------*/
static char shell_buf[256];
static char prompt[] = "\n  ksh> ";

static void shell_run(void) {
    vga_puts_color("\n  Kernel Shell ready. Type 'help' for commands.\n",
                   VGA_LIGHT_GREEN, VGA_BLACK);

    while (true) {
        vga_puts_color(prompt, VGA_LIGHT_GREEN, VGA_BLACK);
        kb_readline(shell_buf, sizeof(shell_buf));

        const char *cmd = k_ltrim(shell_buf);
        if (k_strlen(cmd) == 0) continue;

        if (k_strcmp(cmd, "help")       == 0) { cmd_help();       continue; }
        if (k_strcmp(cmd, "clear")      == 0) { cmd_clear();      continue; }
        if (k_strcmp(cmd, "version")    == 0) { cmd_version();    continue; }
        if (k_strcmp(cmd, "halt")       == 0) { cmd_halt();       continue; }
        if (k_strcmp(cmd, "ps")         == 0) { cmd_ps();         continue; }
        if (k_strcmp(cmd, "demo")       == 0) { cmd_demo();       continue; }
        if (k_strcmp(cmd, "threads")    == 0) { cmd_threads();    continue; }
        if (k_strcmp(cmd, "mutex-demo") == 0) { cmd_mutex_demo(); continue; }
        if (k_strcmp(cmd, "prodcon")    == 0) { cmd_prodcon();    continue; }
        if (k_strcmp(cmd, "meminfo")    == 0) { cmd_meminfo();    continue; }
        if (k_strcmp(cmd, "ls")         == 0) { cmd_ls();         continue; }
        if (k_strcmp(cmd, "about")      == 0) { cmd_about();      continue; }
        if (k_strcmp(cmd, "mem")        == 0) { cmd_mem();        continue; }

        if (k_strncmp(cmd, "touch ", 6) == 0) { cmd_touch(k_ltrim(cmd + 6)); continue; }
        if (k_strcmp(cmd, "touch")      == 0) { cmd_touch("");                continue; }

        if (k_strncmp(cmd, "write ", 6) == 0) { cmd_write(k_ltrim(cmd + 6)); continue; }
        if (k_strcmp(cmd, "write")      == 0) { cmd_write("");                continue; }

        if (k_strncmp(cmd, "cat ", 4)   == 0) { cmd_cat(k_ltrim(cmd + 4));   continue; }
        if (k_strcmp(cmd, "cat")        == 0) { cmd_cat("");                 continue; }

        if (k_strncmp(cmd, "rm ", 3)    == 0) { cmd_rm(k_ltrim(cmd + 3));    continue; }
        if (k_strcmp(cmd, "rm")         == 0) { cmd_rm("");                 continue; }

        if (k_strncmp(cmd, "kill ", 5)  == 0) { cmd_kill(k_ltrim(cmd + 5));  continue; }
        if (k_strcmp(cmd, "kill")       == 0) { cmd_kill("");                continue; }

        if (k_strncmp(cmd, "echo ", 5)  == 0) { cmd_echo(k_ltrim(cmd + 5));  continue; }
        if (k_strcmp(cmd, "echo")       == 0) { cmd_echo("");                 continue; }

        if (k_strncmp(cmd, "colour ", 7)== 0) { cmd_colour(k_ltrim(cmd + 7));continue; }
        if (k_strcmp(cmd, "colour")     == 0) { cmd_colour("");                continue; }

        vga_puts_color("  Unknown command: ", VGA_LIGHT_RED, VGA_BLACK);
        vga_puts(cmd);
        vga_puts("\n  Type 'help' for a list of commands.\n");
    }
}

/* ---------------------------------------------------------------------------
 * Kernel entry point
 * --------------------------------------------------------------------------*/
void kernel_main(void) {
    vga_init();
    kb_init();
    idt_init();
    pit_init(PIT_FREQUENCY_HZ);
    scheduler_init();
    thread_init();
    pmm_init();
    fs_init();

    __asm__ __volatile__("sti");

    print_splash();
    shell_run();

    __asm__ __volatile__("cli; hlt");
}
