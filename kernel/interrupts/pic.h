#ifndef KERNEL_INTERRUPTS_PIC_H
#define KERNEL_INTERRUPTS_PIC_H

/* 8259 PIC: remap IRQs to vectors 32–47, default masks keyboard-only (IRQ1). */
void pic_init(void);

#endif
