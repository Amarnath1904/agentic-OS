#ifndef NET_CONFIG_H
#define NET_CONFIG_H

/*
 * Defaults match Bochs/QEMU slirp (user-mode host networking).
 * The host Gemini bridge is reached through slirp's 10.0.2.2 alias.
 */
#define NE2K_IO_BASE   0x300
#define NE2K_IRQ       5

/* Must match bochsrc ne2k mac= (same order on wire) */
#define NET_MAC_0 0xB0
#define NET_MAC_1 0xC4
#define NET_MAC_2 0x20
#define NET_MAC_3 0x00
#define NET_MAC_4 0x00
#define NET_MAC_5 0x01

/* Host byte order octets — compiled into wire byte order in net.c */
#define NET_IP_A   10
#define NET_IP_B   0
#define NET_IP_C   2
#define NET_IP_D   15

#define NET_GW_A   10
#define NET_GW_B   0
#define NET_GW_C   2
#define NET_GW_D   2

#define NET_MASK_A 255
#define NET_MASK_B 255
#define NET_MASK_C 255
#define NET_MASK_D 0

#define TCP_PROBE_DEFAULT_PORT  53
#define TCP_PROBE_TIMEOUT_SPINS 8000000u

/* Host loopback via Bochs/QEMU slirp. API credentials stay on the host. */
#define AI_BRIDGE_A 10
#define AI_BRIDGE_B 0
#define AI_BRIDGE_C 2
#define AI_BRIDGE_D 2
#define AI_BRIDGE_PORT 8765

#endif
