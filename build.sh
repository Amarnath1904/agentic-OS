#!/bin/bash

# ============================================
# BUILD SCRIPT FOR HARDENED KERNEL CORE
# ============================================

set -e  # Exit on error

# Check for cross-compiler
if ! command -v i686-elf-gcc >/dev/null 2>&1; then
    echo "ERROR: i686-elf-gcc not found!"
    echo "Please install a cross-compiler targeting i686-elf"
    echo "See: https://wiki.osdev.org/GCC_Cross-Compiler"
    exit 1
fi

echo "Building hardened kernel core..."

# Build kernel entry (assembly)
echo "Building kernel entry..."
nasm -f elf32 -o kernel/entry.o kernel/entry.asm

# Build GDT (assembly)
echo "Building GDT..."
nasm -f elf32 -o kernel/gdt.o kernel/gdt.asm

# Build interrupts (assembly)
echo "Building interrupt handlers..."
nasm -f elf32 -o kernel/interrupts.o kernel/interrupts.asm

# Build syscall stub (assembly)
echo "Building syscall stub..."
nasm -f elf32 -o kernel/syscalls/syscall.o kernel/syscalls/syscall.asm

# Build C files
echo "Building C kernel files..."

# Kernel core
i686-elf-gcc -c kernel/kernel.c -o kernel/kernel.o \
    -std=gnu99 -ffreestanding -O2 -Wall -Wextra -fno-stack-protector \
    -I. -Ikernel

# IDT setup
i686-elf-gcc -c kernel/idt.c -o kernel/idt.o \
    -std=gnu99 -ffreestanding -O2 -Wall -Wextra -fno-stack-protector \
    -I. -Ikernel

# Interrupt handlers
i686-elf-gcc -c kernel/interrupts.c -o kernel/interrupts_c.o \
    -std=gnu99 -ffreestanding -O2 -Wall -Wextra -fno-stack-protector \
    -I. -Ikernel

# PIC
i686-elf-gcc -c kernel/interrupts/pic.c -o kernel/interrupts/pic.o \
    -std=gnu99 -ffreestanding -O2 -Wall -Wextra -fno-stack-protector \
    -I. -Ikernel

# Line input
i686-elf-gcc -c kernel/io/line_input.c -o kernel/io/line_input.o \
    -std=gnu99 -ffreestanding -O2 -Wall -Wextra -fno-stack-protector \
    -I. -Ikernel

# Login
i686-elf-gcc -c kernel/auth/login.c -o kernel/auth/login.o \
    -std=gnu99 -ffreestanding -O2 -Wall -Wextra -fno-stack-protector \
    -I. -Ikernel

# Memory manager
i686-elf-gcc -c kernel/memory/memory.c -o kernel/memory/memory.o \
    -std=gnu99 -ffreestanding -O2 -Wall -Wextra -fno-stack-protector \
    -I. -Ikernel

i686-elf-gcc -c kernel/memory/paging.c -o kernel/memory/paging.o \
    -std=gnu99 -ffreestanding -O2 -Wall -Wextra -fno-stack-protector \
    -I. -Ikernel

# Scheduler
i686-elf-gcc -c kernel/scheduler/scheduler.c -o kernel/scheduler/scheduler.o \
    -std=gnu99 -ffreestanding -O2 -Wall -Wextra -fno-stack-protector \
    -I. -Ikernel

# Syscalls
i686-elf-gcc -c kernel/syscalls/syscall.c -o kernel/syscalls/syscall_c.o \
    -std=gnu99 -ffreestanding -O2 -Wall -Wextra -fno-stack-protector \
    -I. -Ikernel

# Drivers
i686-elf-gcc -c kernel/drivers/vga.c -o kernel/drivers/vga.o \
    -std=gnu99 -ffreestanding -O2 -Wall -Wextra -fno-stack-protector \
    -I. -Ikernel

i686-elf-gcc -c kernel/drivers/keyboard.c -o kernel/drivers/keyboard.o \
    -std=gnu99 -ffreestanding -O2 -Wall -Wextra -fno-stack-protector \
    -I. -Ikernel

# Security
i686-elf-gcc -c kernel/security/stack_guard.c -o kernel/security/stack_guard.o \
    -std=gnu99 -ffreestanding -O2 -Wall -Wextra -fno-stack-protector \
    -I. -Ikernel

i686-elf-gcc -c kernel/security/bounds_check.c -o kernel/security/bounds_check.o \
    -std=gnu99 -ffreestanding -O2 -Wall -Wextra -fno-stack-protector \
    -I. -Ikernel

# Link kernel (ELF) then produce a flat binary — the bootloader copies raw bytes to 0x10000; an ELF
# header at 0x10000 executes wrong instructions and breaks paging/IDT state.
echo "Linking kernel..."
i686-elf-gcc -T linker.ld -o kernel.elf -ffreestanding -O2 -nostdlib \
    kernel/entry.o \
    kernel/gdt.o \
    kernel/interrupts.o \
    kernel/kernel.o \
    kernel/idt.o \
    kernel/interrupts_c.o \
    kernel/interrupts/pic.o \
    kernel/io/line_input.o \
    kernel/auth/login.o \
    kernel/memory/memory.o \
    kernel/memory/paging.o \
    kernel/scheduler/scheduler.o \
    kernel/syscalls/syscall.o \
    kernel/syscalls/syscall_c.o \
    kernel/drivers/vga.o \
    kernel/drivers/keyboard.o \
    kernel/security/stack_guard.o \
    kernel/security/bounds_check.o \
    -lgcc

echo "Creating flat kernel.bin (strip ELF headers)..."
i686-elf-objcopy -O binary kernel.elf kernel.bin

# Boot sector must load the full kernel (was 1 sector — too small, causes triple-fault / BIOS loop)
KERNEL_BYTES=$(wc -c < kernel.bin | tr -d ' ')
KERNEL_SECTORS=$(( (KERNEL_BYTES + 511) / 512 ))
if [ "$KERNEL_SECTORS" -lt 1 ]; then
    KERNEL_SECTORS=1
fi
echo "Building bootloader (load $KERNEL_BYTES byte kernel = $KERNEL_SECTORS sectors)..."
nasm -f bin -o boot.bin -DKERNEL_SECTORS=$KERNEL_SECTORS boot.asm

# Remove old image if it exists
rm -f boot.img

# Ensure minimum size for geometry 1/1/63 (63 sectors = 32256 bytes)
MIN_SIZE=32256

# Create disk image by concatenating bootloader and kernel
echo "Creating disk image..."
cat boot.bin kernel.bin > boot.img

# Verify the final image size is a multiple of 512 bytes
if command -v stat >/dev/null 2>&1; then
    IMAGE_SIZE=$(stat -c%s boot.img 2>/dev/null || stat -f%z boot.img 2>/dev/null)
else
    IMAGE_SIZE=$(wc -c < boot.img | tr -d ' ')
fi

if [ -z "$IMAGE_SIZE" ] || [ "$IMAGE_SIZE" -eq 0 ]; then
    echo "Error: Could not determine image size"
    exit 1
fi

echo "Image size: $IMAGE_SIZE bytes"

# Ensure minimum size
if [ "$IMAGE_SIZE" -lt "$MIN_SIZE" ]; then
    PADDING=$((MIN_SIZE - IMAGE_SIZE))
    if command -v dd >/dev/null 2>&1; then
        dd if=/dev/zero bs=1 count=$PADDING >> boot.img 2>/dev/null
    else
        printf '\0%.0s' $(seq 1 $PADDING) >> boot.img 2>/dev/null
    fi
    IMAGE_SIZE=$MIN_SIZE
    echo "Padded to minimum size: $MIN_SIZE bytes"
fi

# Pad to multiple of 512 bytes if needed
if [ $((IMAGE_SIZE % 512)) -ne 0 ]; then
    echo "Warning: Image size ($IMAGE_SIZE) is not a multiple of 512 bytes"
    PADDING=$((512 - (IMAGE_SIZE % 512)))
    if command -v dd >/dev/null 2>&1; then
        dd if=/dev/zero bs=1 count=$PADDING >> boot.img 2>/dev/null
    else
        printf '\0%.0s' $(seq 1 $PADDING) >> boot.img 2>/dev/null
    fi
    echo "Padded image by $PADDING bytes"
    FINAL_SIZE=$((IMAGE_SIZE + PADDING))
    echo "Final image size: $FINAL_SIZE bytes"
else
    echo "Image size is correct (multiple of 512 bytes): $IMAGE_SIZE bytes"
fi

echo "Build complete! boot.img is ready."
