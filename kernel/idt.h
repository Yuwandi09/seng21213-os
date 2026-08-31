/* =============================================================================
 * SENG21213-OS :: Interrupt Descriptor Table (IDT) Header
 * File   : kernel/idt.h
 * ============================================================================*/
#ifndef IDT_H
#define IDT_H

#include "../include/types.h"

/* Structure of an IDT entry (Gate Descriptor) */
struct idt_entry {
    uint16_t base_low;    /* Lower 16 bits of handler address */
    uint16_t sel;         /* Kernel segment selector (0x08) */
    uint8_t  always0;     /* Must be zero */
    uint8_t  flags;       /* Type and attributes (0x8E = 32-bit Interrupt Gate) */
    uint16_t base_high;   /* Upper 16 bits of handler address */
} __attribute__((packed));
typedef struct idt_entry idt_entry_t;

/* IDT pointer passed to LIDT instruction */
struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));
typedef struct idt_ptr idt_ptr_t;

/* Registers frame passed to C interrupt handlers */
struct interrupt_registers {
    uint32_t ds;                                     /* Data segment selector */
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; /* Pushed by pushad */
    uint32_t int_no, err_code;                       /* Interrupt number and error code */
    uint32_t eip, cs, eflags, useresp, ss;           /* Pushed by CPU automatically */
};
typedef struct interrupt_registers registers_t;

typedef void (*isr_handler_t)(registers_t *);

void idt_init(void);
void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags);
void register_interrupt_handler(uint8_t n, isr_handler_t handler);
void pic_send_eoi(uint8_t irq);

#endif /* IDT_H */
