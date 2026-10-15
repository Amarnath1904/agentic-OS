[BITS 16]
[ORG 0x7c00]

; Number of 512-byte sectors to load after the MBR (must cover full kernel.bin).
; build.sh passes -DKERNEL_SECTORS=N after linking kernel.bin.
%ifndef KERNEL_SECTORS
%define KERNEL_SECTORS 80
%warning KERNEL_SECTORS not set; default 80 — run build.sh so the count matches kernel.bin
%endif

; ============================================
; BOOTLOADER - Loads kernel and switches to PM
; ============================================

start:
    ; Initialize segments and stack
    xor ax, ax   
    mov ds, ax
    mov es, ax  
    mov ss, ax
    mov sp, 0x7c00
    
    ; Display loading message
    mov si, loading_msg
    call print_string
    
    ; Load kernel from disk
    call load_kernel
    
    ; Enable A20 line
    call enable_a20
    
    ; Set up GDT
    call setup_gdt
    
    ; Switch to protected mode
    cli
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    
    ; Far jump to 32-bit code segment
    jmp CODE_SEG:start_protected_mode

; ============================================
; MODULE: String Printing (16-bit)
; ============================================
print_string:
    pusha
print_loop:
    lodsb
    cmp al, 0
    je print_done
    mov ah, 0x0E
    mov bh, 0
    int 0x10
    jmp print_loop
print_done:
    popa
    ret

; ============================================
; MODULE: Disk Loading (INT 13h AH=42 LBA read)
; Loads kernel to physical 0x10000 (segment 0x1000:0). First kernel LBA = 1.
; ============================================
load_kernel:
    pusha
    
    mov ah, 0x00
    mov dl, 0x80
    int 0x13
    jc disk_error
    
    ; Extended disk read (EDD) — required for kernels larger than one sector
    mov ah, 0x41
    mov bx, 0x55AA
    mov dl, 0x80
    int 0x13
    jc disk_error_ext
    cmp bx, 0xAA55
    jne disk_error_ext
    
    mov si, dap
    mov ah, 0x42
    mov dl, 0x80
    int 0x13
    jc disk_error
    
    popa
    ret

disk_error_ext:
    mov si, no_ext_msg
    call print_string
    jmp disk_error_halt

disk_error:
    mov si, disk_error_msg
    call print_string
disk_error_halt:
    hlt
    jmp disk_error_halt

; ============================================
; MODULE: A20 Line Enable
; ============================================
enable_a20:
    pusha
    
    ; Try BIOS method first
    mov ax, 0x2401
    int 0x15
    jnc a20_done
    
    ; If BIOS method fails, try keyboard controller method
    call wait_8042_command
    mov al, 0xAD     ; Disable keyboard
    out 0x64, al
    
    call wait_8042_command
    mov al, 0xD0     ; Read output port
    out 0x64, al
    
    call wait_8042_data
    in al, 0x60
    push ax
    
    call wait_8042_command
    mov al, 0xD1     ; Write output port
    out 0x64, al
    
    call wait_8042_command
    pop ax
    or al, 2         ; Set A20 bit
    out 0x60, al
    
    call wait_8042_command
    mov al, 0xAE     ; Enable keyboard
    out 0x64, al
    
    call wait_8042_command

a20_done:
    popa
    ret

wait_8042_command:
    in al, 0x64
    test al, 2
    jnz wait_8042_command
    ret

wait_8042_data:
    in al, 0x64
    test al, 1
    jz wait_8042_data
    ret

; ============================================
; MODULE: GDT Setup
; ============================================
setup_gdt:
    cli
    lgdt [gdt_descriptor]
    ret

; ============================================
; GDT Definition
; ============================================
gdt_start:
    ; Null descriptor
    dd 0x0
    dd 0x0
    
    ; Code segment descriptor
    dw 0xFFFF       ; Limit (bits 0-15)
    dw 0x0          ; Base (bits 0-15)
    db 0x0          ; Base (bits 16-23)
    db 10011010b    ; Access byte
    db 11001111b    ; Flags + Limit (bits 16-19)
    db 0x0          ; Base (bits 24-31)
    
    ; Data segment descriptor
    dw 0xFFFF       ; Limit (bits 0-15)
    dw 0x0          ; Base (bits 0-15)
    db 0x0          ; Base (bits 16-23)
    db 10010010b    ; Access byte
    db 11001111b    ; Flags + Limit (bits 16-19)
    db 0x0          ; Base (bits 24-31)

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1  ; Size of GDT
    dd gdt_start                 ; Start address of GDT

; Selector values (offsets into GDT)
CODE_SEG equ 0x08    ; Code segment selector (offset 8 bytes from GDT start)
DATA_SEG equ 0x10    ; Data segment selector (offset 16 bytes from GDT start)

; ============================================
; 32-bit Protected Mode Entry
; ============================================
[BITS 32]
start_protected_mode:
    ; Set up segment registers for protected mode
    mov ax, DATA_SEG
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    ; Set up stack
    mov ebp, 0x90000
    mov esp, ebp
    
    ; Far jump to kernel entry point at 0x10000
    ; Kernel entry point is _start in entry.asm
    jmp CODE_SEG:0x10000

; ============================================
; Data Section
; ============================================
[BITS 16]
loading_msg:        db "Loading kernel...", 13, 10, 0
disk_error_msg:     db "Disk read error!", 13, 10, 0
no_ext_msg:         db "BIOS INT13 extensions required.", 13, 10, 0

; Disk Address Packet for INT 13h AH=42
dap:
    db 0x10                    ; packet size
    db 0
    dw KERNEL_SECTORS          ; sectors to read
    ; DAP "address" = (segment << 16) | offset — NOT linear! 0x1000:0 => 0x10000000
    dd 0x10000000              ; 0x1000:0x0000 -> physical 0x10000
    dd 1                       ; start LBA (low) — sector after MBR
    dd 0                       ; start LBA (high)

; Boot sector padding and signature
times (0x1be-($-$$)) db 0

    db 80h
    db 0,2,0
    db 0f0h
    db 0ffh,0ffh,0ffh
    dd 1
    dd (20*16*63-1)
	
    times (16*3) db 0

    db 0x55
    db 0xaa
