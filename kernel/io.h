#ifndef KERNEL_IO_H
#define KERNEL_IO_H

#include "kernel.h"

static inline void outb(u16 port, u8 value)
{
    asm volatile("outb %0, %1" :: "a"(value), "Nd"(port));
}

static inline u8 inb(u16 port)
{
    u8 ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outw(u16 port, u16 value)
{
    asm volatile("outw %0, %1" :: "a"(value), "Nd"(port));
}

#endif
