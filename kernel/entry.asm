[BITS 32]
[GLOBAL _start]
[EXTERN kmain]

; ============================================
; KERNEL ENTRY POINT
; Called from bootloader after switching to protected mode
; ============================================

_start:
    ; Set up stack
    mov esp, stack_top
    mov ebp, esp
    
    ; Clear direction flag
    cld
    
    ; Initialize BSS section (zero out uninitialized data)
    ; BSS symbols are defined in linker script
    extern __bss_start
    extern __bss_end
    
    mov edi, __bss_start
    mov ecx, __bss_end
    sub ecx, edi
    mov al, 0
    rep stosb
    
    ; Call C kernel main
    call kmain
    
    ; If kmain returns, halt
    cli
halt_loop:
    hlt
    jmp halt_loop

; ============================================
; Stack Section
; ============================================
section .bss
align 16
stack_bottom:
    resb 16384  ; 16KB stack
stack_top:

