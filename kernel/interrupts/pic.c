/* ============================================
 * 8259 PIC — vector remap for IDT IRQ stubs
 * ============================================ */

#include "pic.h"
#include "../kernel.h"
#include "../io.h"

#define PIC1_CMD  0x20
#define PIC1_DATA 0x21
#define PIC2_CMD  0xA0
#define PIC2_DATA 0xA1

#define ICW1_INIT 0x10
#define ICW1_ICW4 0x01

static void pic_io_wait(void)
{
    outb(0x80, 0);
}

void pic_init(void)
{
    outb(PIC1_CMD, ICW1_INIT | ICW1_ICW4);
    outb(PIC2_CMD, ICW1_INIT | ICW1_ICW4);
    pic_io_wait();

    outb(PIC1_DATA, 0x20);
    outb(PIC2_DATA, 0x28);

    outb(PIC1_DATA, 0x04);
    outb(PIC2_DATA, 0x02);

    outb(PIC1_DATA, 0x01);
    outb(PIC2_DATA, 0x01);

    /* IRQ1 keyboard only on master; slave fully masked */
    outb(PIC1_DATA, 0xFD);
    outb(PIC2_DATA, 0xFF);
}