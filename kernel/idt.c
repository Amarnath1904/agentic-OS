/* ============================================
 * IDT SETUP (C)
 * Interrupt Descriptor Table initialization
 * ============================================ */

#include "kernel.h"

/* IDT entry structure */
struct idt_entry {
    u16 base_low;
    u16 selector;
    u8 always0;
    u8 flags;
    u16 base_high;
} __attribute__((packed));

struct idt_ptr {
    u16 limit;
    u32 base;
} __attribute__((packed));

/* IDT (256 entries) */
struct idt_entry idt[256] __attribute__((aligned(8)));

/* Export idt_pointer for assembly */
struct idt_ptr idt_pointer __attribute__((used));

/* External ISR addresses */
extern void* isr_stub_table[];

/* ============================================
 * Set IDT Entry
 * ============================================ */
void idt_set_gate(u8 num, u32 base, u16 selector, u8 flags)
{
    idt[num].base_low = base & 0xFFFF;
    idt[num].base_high = (base >> 16) & 0xFFFF;
    idt[num].selector = selector;
    idt[num].always0 = 0;
    /* Use flags as-is: 0x8E = 32-bit interrupt gate, DPL=0; 0xEE = trap, DPL=3. */
    idt[num].flags = flags;
}

/* ============================================
 * Initialize IDT
 * ============================================ */
void init_idt_setup(void)
{
    /* Set up IDT pointer */
    idt_pointer.limit = sizeof(idt) - 1;
    idt_pointer.base = (u32)&idt;
    
    /* Zero out IDT */
    for (u32 i = 0; i < 256; i++) {
        idt_set_gate(i, 0, 0x08, 0);
    }
    
    /* Set up exception handlers (0-31) */
    for (u32 i = 0; i < 32; i++) {
        if (isr_stub_table[i] != NULL) {
            idt_set_gate(i, (u32)isr_stub_table[i], 0x08, 0x8E);  /* Interrupt gate */
        }
    }
    
    /* Set up IRQ handlers (32-47) */
    for (u32 i = 32; i < 48; i++) {
        if (isr_stub_table[i] != NULL) {
            idt_set_gate(i, (u32)isr_stub_table[i], 0x08, 0x8E);  /* Interrupt gate */
        }
    }
    
    /* Set up syscall handler (128) */
    extern void syscall_handler_stub(void);
    idt_set_gate(128, (u32)syscall_handler_stub, 0x08, 0xEE);  /* Trap gate, DPL=3 */
}

