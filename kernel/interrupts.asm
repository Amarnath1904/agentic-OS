[BITS 32]

; ============================================
; MODULE: Interrupt Handling
; IDT setup and interrupt entry stubs
; ============================================

[GLOBAL idt_flush]
[GLOBAL isr_stub_table]
[EXTERN idt_pointer]
[EXTERN isr_handler]

; ============================================
; IDT Flush Function
; ============================================
idt_flush:
    lidt [idt_pointer]
    ret

; ============================================
; Common ISR Stub
; Saves all registers and calls C handler
; ============================================
isr_common_stub:
    ; Save all registers
    pusha
    
    ; Save segment registers
    push ds
    push es
    push fs
    push gs
    
    ; Load kernel data segment
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    ; Push interrupt number and call C handler
    push esp  ; Pointer to interrupt frame
    call isr_handler
    add esp, 4
    
    ; Restore segment registers
    pop gs
    pop fs
    pop es
    pop ds
    
    ; Restore all registers
    popa
    
    ; Remove error code and interrupt number from stack
    add esp, 8
    
    ; Return from interrupt
    iret

; ============================================
; Exception Handlers (0-31)
; ============================================
%macro ISR_NOERRCODE 1
[GLOBAL isr%1]
isr%1:
    cli
    push byte 0      ; Dummy error code
    push byte %1     ; Interrupt number
    jmp isr_common_stub
%endmacro

%macro ISR_ERRCODE 1
[GLOBAL isr%1]
isr%1:
    cli
    push byte %1     ; Interrupt number (error code already on stack)
    jmp isr_common_stub
%endmacro

; Exception handlers
ISR_NOERRCODE 0   ; Divide by zero
ISR_NOERRCODE 1   ; Debug
ISR_NOERRCODE 2   ; Non-maskable interrupt
ISR_NOERRCODE 3   ; Breakpoint
ISR_NOERRCODE 4   ; Overflow
ISR_NOERRCODE 5   ; Bound range exceeded
ISR_NOERRCODE 6   ; Invalid opcode
ISR_NOERRCODE 7   ; Device not available
ISR_ERRCODE 8     ; Double fault
ISR_NOERRCODE 9   ; Coprocessor segment overrun
ISR_ERRCODE 10    ; Invalid TSS
ISR_ERRCODE 11    ; Segment not present
ISR_ERRCODE 12    ; Stack fault
ISR_ERRCODE 13    ; General protection fault
ISR_ERRCODE 14    ; Page fault
ISR_NOERRCODE 15  ; Reserved
ISR_NOERRCODE 16  ; x87 FPU error
ISR_ERRCODE 17    ; Alignment check
ISR_NOERRCODE 18  ; Machine check
ISR_NOERRCODE 19  ; SIMD floating point exception
ISR_NOERRCODE 20  ; Virtualization exception
ISR_NOERRCODE 21  ; Control protection exception
ISR_NOERRCODE 22  ; Reserved
ISR_NOERRCODE 23  ; Reserved
ISR_NOERRCODE 24  ; Reserved
ISR_NOERRCODE 25  ; Reserved
ISR_NOERRCODE 26  ; Reserved
ISR_NOERRCODE 27  ; Reserved
ISR_NOERRCODE 28  ; Hypervisor injection exception
ISR_NOERRCODE 29  ; VMM communication exception
ISR_ERRCODE 30    ; Security exception
ISR_NOERRCODE 31  ; Reserved

; ============================================
; IRQ Handlers (32-47)
; ============================================
%macro IRQ 2
[GLOBAL irq%1]
irq%1:
    cli
    push byte 0
    push byte %2
    jmp isr_common_stub
%endmacro

IRQ 0, 32   ; Timer
IRQ 1, 33   ; Keyboard
IRQ 2, 34   ; Cascade
IRQ 3, 35   ; COM2
IRQ 4, 36   ; COM1
IRQ 5, 37   ; LPT2
IRQ 6, 38   ; Floppy
IRQ 7, 39   ; LPT1
IRQ 8, 40   ; CMOS clock
IRQ 9, 41   ; Legacy SCSI
IRQ 10, 42  ; Legacy SCSI
IRQ 11, 43  ; PS/2 mouse
IRQ 12, 44  ; FPU
IRQ 13, 45  ; Primary ATA
IRQ 14, 46  ; Secondary ATA
IRQ 15, 47  ; Reserved

; ============================================
; ISR Stub Table (for dynamic IDT setup)
; ============================================
section .data
[GLOBAL isr_stub_table]
isr_stub_table:
    dd isr0, isr1, isr2, isr3, isr4, isr5, isr6, isr7
    dd isr8, isr9, isr10, isr11, isr12, isr13, isr14, isr15
    dd isr16, isr17, isr18, isr19, isr20, isr21, isr22, isr23
    dd isr24, isr25, isr26, isr27, isr28, isr29, isr30, isr31
    dd irq0, irq1, irq2, irq3, irq4, irq5, irq6, irq7
    dd irq8, irq9, irq10, irq11, irq12, irq13, irq14, irq15
    times 208 dd 0  ; Fill rest with zeros

