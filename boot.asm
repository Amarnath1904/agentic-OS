[BITS 16]
[ORG 0x7c00]

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
; MODULE: Disk Loading
; ============================================
load_kernel:
    pusha
    
    ; Reset disk system first
    mov ah, 0x00    ; Reset disk
    mov dl, 0x80    ; Drive 0x80 (first hard disk)
    int 0x13
    jc disk_error
    
    ; Kernel will be loaded at 0x10000 (64KB)
    ; We'll load it to ES:BX = 0x1000:0x0000
    mov ax, 0x1000
    mov es, ax
    mov bx, 0x0000
    
    ; Read from sector 2 (after boot sector)
    ; With geometry 1/1/2: cylinder 0, head 0, sector 2
    ; Read 1 sector (kernel is 512 bytes = 1 sector)
    mov ah, 0x02    ; Read sectors
    mov al, 1       ; Number of sectors to read
    mov ch, 0       ; Cylinder 0
    mov cl, 2       ; Sector 2 (1-indexed, so 2 is valid)
    mov dh, 0       ; Head 0
    mov dl, 0x80    ; Drive 0x80 (first hard disk)
    
    int 0x13
    jc disk_error
    
    popa
    ret

disk_error:
    mov si, disk_error_msg
    call print_string
    hlt
    jmp disk_error

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
