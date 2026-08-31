; =============================================================================
; SENG21213-OS :: Interrupt Service Routine Assembly Stubs
; File   : kernel/idt_asm.asm
; =============================================================================
[BITS 32]
[GLOBAL idt_flush]
[GLOBAL isr0]
[GLOBAL isr32]
[GLOBAL isr33]
[EXTERN idt_ptr_val]
[EXTERN isr_handler]
[EXTERN irq_handler]

idt_flush:
    lidt [idt_ptr_val]
    ret

isr_common_stub:
    pushad
    mov ax, ds
    push eax
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp
    call isr_handler
    add esp, 4

    pop eax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    popad
    add esp, 8
    iret

irq_common_stub:
    pushad
    mov ax, ds
    push eax
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp
    call irq_handler
    add esp, 4

    pop eax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    popad
    add esp, 8
    iret

isr0:
    push dword 0
    push dword 0
    jmp isr_common_stub

isr32:
    push dword 0
    push dword 32
    jmp irq_common_stub

isr33:
    push dword 0
    push dword 33
    jmp irq_common_stub
