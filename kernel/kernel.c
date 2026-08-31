/* =============================================================================
 * SENG21213-OS :: Main Kernel (Stage 1 – Process Management & Scheduling)
 * File   : kernel/kernel.c
 * ============================================================================*/

#include "vga.h"
#include "keyboard.h"
#include "idt.h"
#include "pit.h"
#include "process.h"
#include "scheduler.h"
#include "../include/types.h"

static void cmd_help(void);
static void cmd_clear(void);
static void cmd_about(void);
static void cmd_version(void);
static void cmd_echo(const char *args);
static void cmd_colour(const char *args);
static void cmd_halt(void);
static void cmd_mem(void);
static void cmd_ps(void);
static void cmd_kill(const char *args);
static void cmd_demo(void);

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

static void print_splash(void) {
    vga_clear(VGA_BLACK);

    vga_draw_box(0, 0, 7, 80, VGA_LIGHT_MAGENTA);

    vga_set_cursor(1, 2);
    vga_puts_color("  SENG21213-OS  |  Computer Architecture & Operating Systems",
                   VGA_YELLOW, VGA_BLACK);

    vga_set_cursor(2, 2);
    vga_puts_color("  Stage 1: Process Table & Round-Robin Scheduler (v0.2-stage1)",
                   VGA_LIGHT_CYAN, VGA_BLACK);

    vga_set_cursor(3, 2);
    vga_puts_color("  Faculty of Engineering – Department of Software Engineering",
                   VGA_LIGHT_GREY, VGA_BLACK);

    vga_set_cursor(4, 2);
    vga_puts_color("  Type 'help' for commands, 'ps' to see processes, 'demo' to test.",
                   VGA_LIGHT_GREEN, VGA_BLACK);

    vga_set_cursor(5, 2);
    vga_puts_color("  PIT: 100 Hz Timer (IRQ0)  |  Scheduler: Round-Robin (20ms quantum)",
                   VGA_DARK_GREY, VGA_BLACK);

    vga_set_cursor(8, 0);
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    vga_puts("  [  OK  ] GDT 32-bit Protected Mode active\n");
    vga_puts("  [  OK  ] VGA driver initialized at 0xB8000\n");
    vga_puts("  [  OK  ] IDT and 8259 PIC remapped (Vectors 32-47)\n");
    vga_puts("  [  OK  ] i8253 PIT timer configured at 100 Hz (IRQ0)\n");
    vga_puts("  [  OK  ] Round-Robin process scheduler active\n\n");
}

static void worker1_task(void) {
    for (int i = 0; i < 10; i++) {
        vga_puts_color(" [W1] ", VGA_LIGHT_CYAN, VGA_BLACK);
        pit_sleep(20);
    }
    vga_puts_color(" [Worker 1 Done] ", VGA_LIGHT_GREEN, VGA_BLACK);
    process_exit();
}

static void worker2_task(void) {
    for (int i = 0; i < 10; i++) {
        vga_puts_color(" [W2] ", VGA_LIGHT_MAGENTA, VGA_BLACK);
        pit_sleep(30);
    }
    vga_puts_color(" [Worker 2 Done] ", VGA_LIGHT_GREEN, VGA_BLACK);
    process_exit();
}

static void cmd_help(void) {
    vga_puts_color("\n  SENG21213-OS Shell Commands (Stage 1)\n", VGA_YELLOW, VGA_BLACK);
    vga_puts("  ─────────────────────────────────────────────────────────\n");
    vga_puts("  help             – Show this help message\n");
    vga_puts("  clear            – Clear the screen\n");
    vga_puts("  echo <text>      – Echo text to screen\n");
    vga_puts("  version          – Print OS kernel version\n");
    vga_puts("  colour <fg> <bg> – Set text colours (0-15)\n");
    vga_puts("  halt             – Halt CPU\n");
    vga_puts("  ps               – [Stage 1] List all processes and states\n");
    vga_puts("  kill <pid>       – [Stage 1] Terminate a process\n");
    vga_puts("  demo             – [Stage 1] Launch concurrent worker processes\n");
    vga_puts("  about            – About this OS and course\n");
    vga_puts("  mem              – Memory map info\n");
    vga_puts_color("\n  Upcoming Milestones (Stages 2–4):\n", VGA_LIGHT_CYAN, VGA_BLACK);
    vga_puts("  threads          – [Stage 2] Kernel threads & synchronization\n");
    vga_puts("  meminfo          – [Stage 3] Physical page frame manager\n");
    vga_puts("  ls / touch / cat – [Stage 4] RAM-disk file system\n\n");
}

static void cmd_clear(void) {
    vga_clear(VGA_BLACK);
}

static void cmd_about(void) {
    vga_puts_color("\n  About SENG21213-OS\n", VGA_LIGHT_CYAN, VGA_BLACK);
    vga_puts("  ─────────────────────────────────────────────\n");
    vga_puts("  Architecture : x86 (i686), 32-bit Protected Mode\n");
    vga_puts("  Scheduler    : Preemptive Round-Robin (100 Hz PIT timer)\n");
    vga_puts("  Processes    : PCB Table (16 tasks, 4 KB dedicated stacks)\n");
    vga_puts("  Bootloader   : Custom MBR (NASM)\n");
    vga_puts("  Kernel       : Freestanding C (GCC, no libc)\n");
    vga_puts("  VM Target    : QEMU (qemu-system-i386)\n");
    vga_puts("  Course       : SENG 21213 – Computer Architecture & OS\n\n");
}

static void cmd_echo(const char *args) {
    vga_puts("  ");
    vga_puts(args);
    vga_puts("\n");
}

static void cmd_version(void) {
    vga_puts_color("\n  SENG21213-OS v0.2-stage1\n", VGA_LIGHT_CYAN, VGA_BLACK);
    vga_puts("  Stage 1: Process Table & Round-Robin Scheduler\n");
    vga_puts("  Architecture : x86 (i686) 32-bit Protected Mode\n\n");
}

static void cmd_colour(const char *args) {
    const char *p = k_ltrim(args);
    if (!*p) {
        vga_puts_color("  Usage: colour <fg 0-15> <bg 0-15>\n", VGA_YELLOW, VGA_BLACK);
        vga_puts("  Example: colour 14 0 (Yellow on Black)\n");
        return;
    }
    int fg = k_atoi(&p);
    p = k_ltrim(p);
    int bg = 0;
    if (*p) {
        bg = k_atoi(&p);
    }
    if (fg < 0 || fg > 15 || bg < 0 || bg > 15) {
        vga_puts_color("  Colours must be in range 0-15.\n", VGA_LIGHT_RED, VGA_BLACK);
        return;
    }
    vga_set_color((vga_color_t)fg, (vga_color_t)bg);
    vga_puts("  Colour updated.\n");
}

static void cmd_halt(void) {
    vga_puts_color("\n  System halted. It is now safe to power off.\n", VGA_LIGHT_RED, VGA_BLACK);
    __asm__ __volatile__("cli; hlt");
    while (1) { __asm__ __volatile__("hlt"); }
}

static void cmd_mem(void) {
    vga_puts_color("\n  Memory Map\n", VGA_LIGHT_CYAN, VGA_BLACK);
    vga_puts("  ─────────────────────────────────────────────\n");
    vga_puts("  0x00000000 – 0x000FFFFF  :  First 1 MB (reserved/BIOS/VGA)\n");
    vga_puts("  0x00010000               :  Kernel Entry Point (0x10000)\n");
    vga_puts("  0x00090000               :  Kernel Main Stack Base\n");
    vga_puts("  0x00100000 – 0x00EFFFFF  :  Extended Memory (Usable)\n");
    vga_puts("  0x000B8000               :  VGA Video Buffer\n\n");
}

static const char *state_to_str(proc_state_t st) {
    switch (st) {
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
            vga_printf("  %d     ", table[i].pid);
            const char *st = state_to_str(table[i].state);
            vga_puts(st);
            int st_len = k_strlen(st);
            for (int s = 0; s < 13 - st_len; s++) vga_putchar(' ');
            vga_printf("%d", table[i].ticks);
            int ticks_len = 1;
            uint32_t t = table[i].ticks;
            while (t >= 10) { ticks_len++; t /= 10; }
            for (int s = 0; s < 8 - ticks_len; s++) vga_putchar(' ');
            vga_printf("%s\n", table[i].name);
        }
    }
    vga_puts("\n");
}

static void cmd_kill(const char *args) {
    const char *p = k_ltrim(args);
    if (!*p) {
        vga_puts_color("  Usage: kill <pid>\n", VGA_YELLOW, VGA_BLACK);
        return;
    }
    int pid = k_atoi(&p);
    process_kill((uint32_t)pid);
}

static void cmd_demo(void) {
    vga_puts_color("\n  [Stage 1 Demo] Spawning 2 concurrent processes...\n", VGA_YELLOW, VGA_BLACK);
    vga_puts("  - worker1: prints [W1] every 200 ms\n");
    vga_puts("  - worker2: prints [W2] every 300 ms\n");
    vga_puts("  They run concurrently alongside this interactive shell.\n");
    vga_puts("  Type 'ps' while they run to observe tick counts advancing!\n\n");

    process_create("worker1", worker1_task);
    process_create("worker2", worker2_task);
}

static char  shell_buf[256];
static char  prompt[] = "\n  ksh> ";

static void shell_run(void) {
    vga_puts_color("\n  Kernel Shell ready. Type 'help' for commands, 'demo' for multi-tasking.\n",
                   VGA_LIGHT_GREEN, VGA_BLACK);

    while (true) {
        vga_puts_color(prompt, VGA_LIGHT_GREEN, VGA_BLACK);
        kb_readline(shell_buf, sizeof(shell_buf));

        const char *cmd = k_ltrim(shell_buf);
        if (k_strlen(cmd) == 0) continue;

        if (k_strcmp(cmd, "help")    == 0) { cmd_help();    continue; }
        if (k_strcmp(cmd, "clear")   == 0) { cmd_clear();   continue; }
        if (k_strcmp(cmd, "version") == 0) { cmd_version(); continue; }
        if (k_strcmp(cmd, "halt")    == 0) { cmd_halt();    continue; }
        if (k_strcmp(cmd, "ps")      == 0) { cmd_ps();      continue; }
        if (k_strcmp(cmd, "demo")    == 0) { cmd_demo();    continue; }
        if (k_strcmp(cmd, "about")   == 0) { cmd_about();   continue; }
        if (k_strcmp(cmd, "mem")     == 0) { cmd_mem();     continue; }

        if (k_strncmp(cmd, "kill ", 5) == 0) {
            cmd_kill(k_ltrim(cmd + 5));
            continue;
        }
        if (k_strcmp(cmd, "kill") == 0) {
            cmd_kill("");
            continue;
        }

        if (k_strncmp(cmd, "echo ", 5) == 0) {
            cmd_echo(k_ltrim(cmd + 5));
            continue;
        }
        if (k_strcmp(cmd, "echo") == 0) {
            cmd_echo("");
            continue;
        }

        if (k_strncmp(cmd, "colour ", 7) == 0) {
            cmd_colour(k_ltrim(cmd + 7));
            continue;
        }
        if (k_strcmp(cmd, "colour") == 0) {
            cmd_colour("");
            continue;
        }

        if (k_strcmp(cmd, "threads") == 0 ||
            k_strcmp(cmd, "meminfo") == 0 ||
            k_strcmp(cmd, "free")    == 0 ||
            k_strcmp(cmd, "ls")      == 0 ||
            k_strcmp(cmd, "touch")   == 0 ||
            k_strcmp(cmd, "write")   == 0 ||
            k_strcmp(cmd, "rm")      == 0 ||
            k_strcmp(cmd, "cat")     == 0) {
            vga_puts_color("  [TODO] This command belongs to a later stage.\n",
                           VGA_YELLOW, VGA_BLACK);
            continue;
        }

        vga_puts_color("  Unknown command: ", VGA_LIGHT_RED, VGA_BLACK);
        vga_puts(cmd);
        vga_puts("\n  Type 'help' for a list of commands.\n");
    }
}

void kernel_main(void) {
    vga_init();
    kb_init();
    idt_init();
    pit_init(PIT_FREQUENCY_HZ);
    scheduler_init();

    __asm__ __volatile__("sti");

    print_splash();
    shell_run();

    __asm__ __volatile__("cli; hlt");
}
