[BITS 32]

; ============================================
; MODULE: GDT Setup
; Enhanced GDT with privilege levels
; ============================================

[GLOBAL gdt_flush]
[EXTERN gdt_ptr]

gdt_flush:
    lgdt [gdt_ptr]
    
    ; Reload segment registers
    mov ax, 0x10    ; Data segment selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    ; Far jump to reload code segment
    jmp 0x08:flush_cs
flush_cs:
    ret

; ============================================
; GDT Definition
; ============================================
section .data
align 16

gdt_start:
    ; Null descriptor (0x00)
    dd 0x0
    dd 0x0
    
    ; Kernel code segment (0x08) - Ring 0
    dw 0xFFFF       ; Limit (bits 0-15)
    dw 0x0000       ; Base (bits 0-15)
    db 0x00         ; Base (bits 16-23)
    db 10011010b    ; Access: Present=1, DPL=00, Code=1, Conforming=0, Readable=1, Accessed=0
    db 11001111b    ; Flags: Granularity=1, 32-bit=1, Limit (bits 16-19)=1111
    db 0x00         ; Base (bits 24-31)
    
    ; Kernel data segment (0x10) - Ring 0
    dw 0xFFFF       ; Limit
    dw 0x0000       ; Base
    db 0x00         ; Base
    db 10010010b    ; Access: Present=1, DPL=00, Code=0, Expand down=0, Writable=1, Accessed=0
    db 11001111b    ; Flags
    db 0x00         ; Base
    
    ; User code segment (0x18) - Ring 3
    dw 0xFFFF       ; Limit
    dw 0x0000       ; Base
    db 0x00         ; Base
    db 11111010b    ; Access: Present=1, DPL=11, Code=1, Conforming=0, Readable=1, Accessed=0
    db 11001111b    ; Flags
    db 0x00         ; Base
    
    ; User data segment (0x20) - Ring 3
    dw 0xFFFF       ; Limit
    dw 0x0000       ; Base
    db 0x00         ; Base
    db 11110010b    ; Access: Present=1, DPL=11, Code=0, Expand down=0, Writable=1, Accessed=0
    db 11001111b    ; Flags
    db 0x00         ; Base
    
    ; TSS segment (0x28) - For task switching
    dw 0x0068       ; Limit (104 bytes for TSS)
    dw tss_base     ; Base (low 16 bits)
    db 0x00         ; Base (bits 16-23)
    db 10001001b    ; Access: Present=1, DPL=00, Type=1001 (32-bit TSS)
    db 00000000b    ; Flags: Granularity=0, Limit (bits 16-19)=0000
    db 0x00         ; Base (bits 24-31)

gdt_end:

; GDT pointer structure
gdt_ptr:
    dw gdt_end - gdt_start - 1  ; Size of GDT
    dd gdt_start                 ; Start address of GDT

; TSS base address (placeholder, will be set up later)
tss_base: dd 0

