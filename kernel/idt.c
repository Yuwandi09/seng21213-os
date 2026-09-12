/* =============================================================================
 * SENG21213-OS :: IDT & PIC Implementation
 * File   : kernel/idt.c
 * ============================================================================*/
#include "idt.h"
#include "vga.h"

#define PIC1_CMD  0x20
#define PIC1_DATA 0x21
#define PIC2_CMD  0xA0
#define PIC2_DATA 0xA1

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ __volatile__("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t val;
    __asm__ __volatile__("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

static inline void io_wait(void) {
    outb(0x80, 0);
}

static idt_entry_t idt_entries[256];
idt_ptr_t idt_ptr_val;
static isr_handler_t interrupt_handlers[256];

extern void idt_flush(void);
extern void isr0(void);
extern void isr32(void);
extern void isr33(void);

void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt_entries[num].base_low  = (uint16_t)(base & 0xFFFF);
    idt_entries[num].base_high = (uint16_t)((base >> 16) & 0xFFFF);
    idt_entries[num].sel       = sel;
    idt_entries[num].always0   = 0;
    idt_entries[num].flags     = flags;
}

void register_interrupt_handler(uint8_t n, isr_handler_t handler) {
    interrupt_handlers[n] = handler;
}

static void pic_remap(void) {
    uint8_t a1 = inb(PIC1_DATA);
    uint8_t a2 = inb(PIC2_DATA);
    (void)a1;
    (void)a2;

    outb(PIC1_CMD, 0x11);
    io_wait();
    outb(PIC2_CMD, 0x11);
    io_wait();

    outb(PIC1_DATA, 0x20); /* Master PIC offset 32 */
    io_wait();
    outb(PIC2_DATA, 0x28); /* Slave PIC offset 40 */
    io_wait();

    outb(PIC1_DATA, 0x04); /* Master has slave on IRQ2 */
    io_wait();
    outb(PIC2_DATA, 0x02); /* Slave identity */
    io_wait();

    outb(PIC1_DATA, 0x01); /* 8086 mode */
    io_wait();
    outb(PIC2_DATA, 0x01);
    io_wait();

    outb(PIC1_DATA, 0xFC); /* Unmask IRQ0 and IRQ1 */
    outb(PIC2_DATA, 0xFF); /* Mask slave */
}

void pic_send_eoi(uint8_t irq) {
    if (irq >= 8) {
        outb(PIC2_CMD, 0x20);
    }
    outb(PIC1_CMD, 0x20);
}

void idt_init(void) {
    idt_ptr_val.limit = (sizeof(idt_entry_t) * 256) - 1;
    idt_ptr_val.base  = (uint32_t)&idt_entries;

    for (int i = 0; i < 256; i++) {
        idt_set_gate((uint8_t)i, 0, 0, 0);
        interrupt_handlers[i] = NULL;
    }

    pic_remap();

    idt_set_gate(0, (uint32_t)isr0, 0x08, 0x8E);
    idt_set_gate(32, (uint32_t)isr32, 0x08, 0x8E);
    idt_set_gate(33, (uint32_t)isr33, 0x08, 0x8E);

    idt_flush();
}

void isr_handler(registers_t *regs) {
    if (interrupt_handlers[regs->int_no] != NULL) {
        interrupt_handlers[regs->int_no](regs);
    } else {
        vga_printf("Exception: %d\n", regs->int_no);
    }
}

void irq_handler(registers_t *regs) {
    /* Send EOI before handler in case handler switches context */
    pic_send_eoi((uint8_t)(regs->int_no - 32));
    if (interrupt_handlers[regs->int_no] != NULL) {
        interrupt_handlers[regs->int_no](regs);
    }
}
