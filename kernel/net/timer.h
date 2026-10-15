#ifndef NET_TIMER_H
#define NET_TIMER_H
#include "../kernel.h"
/* PIT channel 0, 100 Hz; deadlines use unsigned subtraction for wraparound. */
void net_timer_init(void);
void net_timer_tick(void);
u32 net_ticks(void);
#endif
