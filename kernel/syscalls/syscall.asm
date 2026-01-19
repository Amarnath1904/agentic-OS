[BITS 32]

; ============================================
; MODULE: Syscall Entry Stub
; INT 0x80 handler
; ============================================

[GLOBAL syscall_handler_stub]
[EXTERN syscall_handler]

syscall_handler_stub:
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
    
    ; Push syscall parameters
    ; Parameters are in: EAX=syscall_num, EBX=arg1, ECX=arg2, EDX=arg3, ESI=arg4, EDI=arg5
    push edi  ; arg5
    push esi  ; arg4
    push edx  ; arg3
    push ecx  ; arg2
    push ebx  ; arg1
    push eax  ; syscall_num
    
    ; Call C handler
    call syscall_handler
    
    ; Clean up stack (6 parameters)
    add esp, 24
    
    ; Return value is in EAX, restore other registers
    mov [esp + 32], eax  ; Store return value in saved EAX
    
    ; Restore segment registers
    pop gs
    pop fs
    pop es
    pop ds
    
    ; Restore all registers (EAX will have return value)
    popa
    
    ; Return from interrupt
    iret

