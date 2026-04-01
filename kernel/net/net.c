#include "net.h"
#include "net_config.h"
#include "net_endian.h"
#include "ne2k.h"

u32 net_our_ipv4(void)
{
    return net_ipv4_from_octets(NET_IP_A, NET_IP_B, NET_IP_C, NET_IP_D);
}

u32 net_gw_ipv4(void)
{
    return net_ipv4_from_octets(NET_GW_A, NET_GW_B, NET_GW_C, NET_GW_D);
}

u32 net_netmask(void)
{
    return net_ipv4_from_octets(NET_MASK_A, NET_MASK_B, NET_MASK_C, NET_MASK_D);
}

bool net_init(void)
{
    return ne2k_init();
}
