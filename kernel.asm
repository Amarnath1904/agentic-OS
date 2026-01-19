[BITS 32]
[ORG 0x10000]

; ============================================
; KERNEL - Minimal stub to verify boot
; ============================================

kernel_start:
    ; Set up stack
    mov ebp, 0x90000
    mov esp, ebp
    
    ; Clear screen (text mode)
    call clear_screen
    
    ; Print kernel loaded message
    mov esi, kernel_msg
    mov edi, 0xB8000    ; Video memory start
    call print_string_pm
    
    ; Wait 3 seconds
    mov eax, 3
    call delay_seconds
    
    ; Print success message
    mov esi, success_msg
    mov edi, 0xB8000 + 160  ; Next line (80 chars * 2 bytes)
    call print_string_pm
    
    ; Infinite loop
kernel_loop:
    hlt
    jmp kernel_loop

; ============================================
; MODULE: Clear Screen
; ============================================
clear_screen:
    pusha
    mov edi, 0xB8000
    mov ecx, 80 * 25    ; 80 columns * 25 rows
    mov ax, 0x0720      ; White on black, space character
    rep stosw
    popa
    ret

; ============================================
; MODULE: Print String (Protected Mode)
; ============================================
; Parameters: ESI = string address, EDI = video memory address
print_string_pm:
    pusha
    mov ah, 0x07        ; White on black
    
print_pm_loop:
    lodsb               ; Load byte from [ESI] into AL, increment ESI
    cmp al, 0
    je print_pm_done
    
    mov [edi], ax       ; Write character and attribute
    add edi, 2          ; Move to next character position
    
    jmp print_pm_loop
    
print_pm_done:
    popa
    ret

; ============================================
; MODULE: Delay Function
; ============================================
; Parameters: EAX = number of seconds to delay
delay_seconds:
    pusha
    
    ; Calculate delay iterations (approximate)
    ; For ~4MHz CPU: targeting ~3 seconds
    ; Each outer loop iteration ≈ 20-30 cycles
    ; 200,000 iterations × 25 cycles ≈ 5,000,000 cycles ≈ 1.25 seconds per second parameter
    mov ebx, 200000      ; Base iterations per second (adjustable)
    mul ebx              ; EAX = EAX * EBX (total iterations)
    mov ecx, eax         ; Outer loop counter
    
delay_outer_loop:
    mov edx, 20          ; Inner loop counter (increased for more cycles)
delay_inner_loop:
    dec edx
    jnz delay_inner_loop
    dec ecx
    jnz delay_outer_loop
    
    popa
    ret

; ============================================
; Data Section
; ============================================
kernel_msg:        db "Kernel loaded successfully!", 0
success_msg:       db "Running in 32-bit protected mode!", 0

; Pad kernel to multiple of 512 bytes (sector size)
; This ensures the disk image size is always a multiple of 512 bytes
kernel_end:
padding_size equ (512 - ((kernel_end - $$) % 512)) % 512
times padding_size db 0

