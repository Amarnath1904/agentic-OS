/* ============================================
 * KERNEL CORE
 * Main kernel logic and initialization
 * ============================================ */

#include "kernel.h"
#include "memory/memory.h"
#include "scheduler/scheduler.h"
#include "drivers/vga.h"

/* Forward declarations */
void kernel_loop(void);

/* ============================================
 * Kernel Main Entry Point
 * ============================================ */
void kmain(void)
{
    /* Initialize hardware */
    init_hardware();
    
    /* Initialize memory management */
    init_memory();
    
    /* Initialize interrupt system */
    init_interrupts();
    
    /* Initialize scheduler */
    init_scheduler();
    
    /* Display kernel banner */
    vga_clear();
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_print("Hardened Kernel Core v1.0\n");
    vga_set_color(VGA_COLOR_LIGHT_GRAY, VGA_COLOR_BLACK);
    vga_print("Initialization complete.\n");
    vga_print("System ready.\n");
    
    /* Main kernel loop */
    kernel_loop();
}

/* ============================================
 * Hardware Initialization
 * ============================================ */
void init_hardware(void)
{
    /* Initialize VGA */
    vga_init();
    
    /* TODO: Initialize other hardware */
}

/* ============================================
 * Interrupt System Initialization
 * ============================================ */
void init_interrupts(void)
{
    extern void init_idt_setup(void);
    extern void idt_flush(void);
    extern void gdt_flush(void);
    
    /* Set up GDT */
    gdt_flush();
    
    /* Set up IDT */
    init_idt_setup();
    idt_flush();
}

/* ============================================
 * Memory System Initialization
 * ============================================ */
void init_memory(void)
{
    extern void init_memory_manager(void);
    extern void init_paging(void);
    
    /* Initialize memory manager */
    init_memory_manager();
    
    /* Initialize paging */
    init_paging();
}

/* ============================================
 * Scheduler Initialization
 * ============================================ */
void init_scheduler(void)
{
    /* Scheduler will be initialized here */
    /* This is a placeholder */
}

/* ============================================
 * Main Kernel Loop
 * ============================================ */
void kernel_loop(void)
{
    while (1) {
        /* Process scheduler */
        scheduler_tick();
        
        /* Handle pending interrupts */
        asm volatile("sti");  /* Enable interrupts */
        asm volatile("hlt");  /* Halt until interrupt */
        asm volatile("cli");  /* Disable interrupts */
    }
}

/* ============================================
 * Panic Handler
 * ============================================ */
void panic(const char* message)
{
    vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
    vga_print("\nKERNEL PANIC: ");
    vga_print(message);
    vga_print("\nSystem halted.\n");
    
    halt();
}

/* ============================================
 * Halt System
 * ============================================ */
void halt(void)
{
    asm volatile("cli");
    while (1) {
        asm volatile("hlt");
    }
}

