#ifndef NET_H
#define NET_H

#include "../kernel.h"

bool net_init(void);
u32 net_our_ipv4(void);
u32 net_gw_ipv4(void);
u32 net_netmask(void);

#endif
