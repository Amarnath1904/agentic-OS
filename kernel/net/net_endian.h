#ifndef NET_ENDIAN_H
#define NET_ENDIAN_H

#include "../kernel.h"

static inline u16 net_htons(u16 v)
{
    return (u16)((v << 8) | (v >> 8));
}

static inline u16 net_ntohs(u16 v)
{
    return net_htons(v);
}

static inline u32 net_ipv4_from_octets(u8 a, u8 b, u8 c, u8 d)
{
    return ((u32)a << 24) | ((u32)b << 16) | ((u32)c << 8) | (u32)d;
}

#endif
