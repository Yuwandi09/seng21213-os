/* =============================================================================
 * SENG21213-OS :: Programmable Interval Timer (PIT / 8253) Implementation
 * File   : kernel/pit.c
 * ============================================================================*/
#include "pit.h"
#include "idt.h"
#include "scheduler.h"

#define PIT_CHANNEL0_DATA 0x40
#define PIT_COMMAND_REG   0x43
#define PIT_BASE_FREQ     1193180

static volatile uint32_t system_ticks = 0;

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ __volatile__("outb %0, %1" : : "a"(val), "Nd"(port));
}

static void timer_callback(registers_t *regs) {
    (void)regs;
    system_ticks++;
    scheduler_tick();
}

void pit_init(uint32_t frequency) {
    register_interrupt_handler(32, timer_callback);

    uint32_t divisor = PIT_BASE_FREQ / frequency;

    outb(PIT_COMMAND_REG, 0x36); /* Channel 0, LSB then MSB, Mode 3 (Square wave) */
    outb(PIT_CHANNEL0_DATA, (uint8_t)(divisor & 0xFF));
    outb(PIT_CHANNEL0_DATA, (uint8_t)((divisor >> 8) & 0xFF));
}

uint32_t pit_get_ticks(void) {
    return system_ticks;
}

void pit_sleep(uint32_t ticks) {
    uint32_t end = system_ticks + ticks;
    while (system_ticks < end) {
        __asm__ __volatile__("hlt");
    }
}
