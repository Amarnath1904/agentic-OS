#include "timer.h"
#include "../io.h"
static volatile u32 ticks;
void net_timer_init(void)
{
    outb(0x43, 0x36);
    outb(0x40, (u8)(11932u & 255u));
    outb(0x40, (u8)(11932u >> 8));
}
void net_timer_tick(void) { ticks++; }
u32 net_ticks(void) { return ticks; }
