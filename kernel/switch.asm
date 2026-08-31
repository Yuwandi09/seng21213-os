; =============================================================================
; SENG21213-OS :: Context Switch Assembly Stub
; File   : kernel/switch.asm
; Purpose: Saves current CPU state and loads next process stack pointer
; =============================================================================
[BITS 32]
[GLOBAL switch_to]

; void switch_to(uint32_t *prev_esp, uint32_t next_esp)
; [esp + 24]: uint32_t *prev_esp
; [esp + 28]: uint32_t next_esp
switch_to:
    push ebp
    push ebx
    push esi
    push edi
    pushfd

    mov eax, [esp + 24] ; Pointer to prev->esp
    mov edx, [esp + 28] ; Value of next->esp

    mov [eax], esp      ; Save current ESP
    mov esp, edx        ; Switch stack to next process

    popfd
    pop edi
    pop esi
    pop ebx
    pop ebp
    ret
