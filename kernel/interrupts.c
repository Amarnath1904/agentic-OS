/* ============================================
 * INTERRUPT HANDLERS (C)
 * C-level interrupt handling
 * ============================================ */

#include "kernel.h"
#include "drivers/keyboard.h"

/* Forward declarations */
static void outb(u16 port, u8 value);
extern void panic(const char* message);

/* ============================================
 * Interrupt Frame Structure
 * ============================================ */
typedef struct {
    u32 gs, fs, es, ds;
    u32 edi, esi, ebp, esp, ebx, edx, ecx, eax;
    u32 int_no, err_code;
    u32 eip, cs, eflags, useresp, ss;
} interrupt_frame_t;

/* ============================================
 * ISR Handler (called from assembly)
 * ============================================ */
void isr_handler(interrupt_frame_t* frame)
{
    if (frame == NULL) {
        return;
    }
    
    u32 int_no = frame->int_no;
    
    /* Handle exceptions */
    if (int_no < 32) {
        switch (int_no) {
            case 0:
                panic("Divide by zero exception!");
                break;
            case 8:
                panic("Double fault!");
                break;
            case 13:
                panic("General protection fault!");
                break;
            case 14:
                panic("Page fault!");
                break;
            default:
                /* Other exceptions */
                break;
        }
    }
    /* Handle IRQs */
    else if (int_no >= 32 && int_no < 48) {
        u32 irq = int_no - 32;
        
        switch (irq) {
            case 0:
                /* Timer interrupt */
                /* Handled by scheduler */
                break;
            case 1:
                /* Keyboard interrupt */
                keyboard_handler();
                break;
            default:
                /* Other IRQs */
                break;
        }
        
        /* Send EOI to PIC */
        if (irq >= 8) {
            outb(0xA0, 0x20);  /* Slave PIC */
        }
        outb(0x20, 0x20);  /* Master PIC */
    }
}

/* ============================================
 * Initialize IDT
 * ============================================ */
void init_idt(void)
{
    extern void init_idt_setup(void);
    extern void idt_flush(void);
    
    /* Set up IDT entries */
    init_idt_setup();
    
    /* Load IDT */
    idt_flush();
    
    /* Enable interrupts */
    asm volatile("sti");
}

