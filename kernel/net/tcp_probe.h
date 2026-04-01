#ifndef TCP_PROBE_H
#define TCP_PROBE_H

#include "../kernel.h"

/* TCP connect probe (SYN -> SYN-ACK -> RST). Uses net_config + NE2K. */
bool tcp_probe_host_port(u32 dst_ip_nbo, u16 dst_port_host);

#endif
