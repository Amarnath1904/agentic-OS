#ifndef NET_UDP_H
#define NET_UDP_H
#include "../kernel.h"
#define UDP_MAX_PAYLOAD 1200u
bool udp_resolve(u32 ip, u8 mac[6]);
bool udp_send(u32 ip, const u8 mac[6], u16 port, u16 local_port,
              const u8* data, u16 len);
/* Poll once, servicing ARP and accepting only the selected peer/ports. */
u16 udp_receive(u32 ip, u16 port, u16 local_port, u8* data, u16 cap);
#endif
