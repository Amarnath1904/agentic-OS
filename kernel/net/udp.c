/* Bounded Ethernet/ARP/IPv4/UDP transport. No IP fragmentation. */
#include "udp.h"
#include "net.h"
#include "net_config.h"
#include "ne2k.h"
#include "ipv4.h"
#include "timer.h"
#include "../string/kstring.h"

static u8 tx[1514], rx[1514];
static const u8 own_mac[6] = {
    NET_MAC_0, NET_MAC_1, NET_MAC_2, NET_MAC_3, NET_MAC_4, NET_MAC_5
};
static u16 ident;
static u16 rd16(const u8* p) { return ((u16)p[0] << 8) | p[1]; }
static u32 rd32(const u8* p) { return ((u32)rd16(p) << 16) | rd16(p + 2); }
static void wr16(u8* p, u16 v) { p[0] = v >> 8; p[1] = v; }
static void wr32(u8* p, u32 v) { wr16(p, v >> 16); wr16(p + 2, v); }
static void ethernet(const u8* mac, u16 type)
{
    kmemset(tx, 0, sizeof(tx));
    kmemcpy(tx, mac, 6);
    kmemcpy(tx + 6, own_mac, 6);
    wr16(tx + 12, type);
}
static bool arp_valid(u16 n)
{
    return n >= 42 && rd16(rx + 12) == 0x0806 && rd16(rx + 14) == 1
        && rd16(rx + 16) == 0x0800 && rx[18] == 6 && rx[19] == 4
        && rd32(rx + 38) == net_our_ipv4();
}
static void arp_reply(u16 n)
{
    if (!arp_valid(n) || rd16(rx + 20) != 1) return;
    ethernet(rx + 22, 0x0806);
    kmemcpy(tx + 14, rx + 14, 8);
    wr16(tx + 20, 2);
    kmemcpy(tx + 22, own_mac, 6);
    wr32(tx + 28, net_our_ipv4());
    kmemcpy(tx + 32, rx + 22, 6);
    kmemcpy(tx + 38, rx + 28, 4);
    (void)ne2k_send(tx, 60);
}
bool udp_resolve(u32 ip, u8 mac[6])
{
    const u8 broadcast[6] = {255,255,255,255,255,255};
    u32 hop = ((ip & net_netmask()) == (net_our_ipv4() & net_netmask()))
        ? ip : net_gw_ipv4();
    for (u32 attempt = 0; attempt < 3; attempt++) {
        ethernet(broadcast, 0x0806);
        wr16(tx + 14, 1); wr16(tx + 16, 0x0800);
        tx[18] = 6; tx[19] = 4; wr16(tx + 20, 1);
        kmemcpy(tx + 22, own_mac, 6);
        wr32(tx + 28, net_our_ipv4()); wr32(tx + 38, hop);
        if (!ne2k_send(tx, 60)) return false;
        u32 start = net_ticks();
        while ((u32)(net_ticks() - start) < 100u) {
            u16 n = ne2k_recv(rx, sizeof(rx));
            if (arp_valid(n) && rd16(rx + 20) == 2 && rd32(rx + 28) == hop) {
                kmemcpy(mac, rx + 22, 6);
                return true;
            }
            arp_reply(n);
        }
    }
    return false;
}
static u16 udp_checksum(const u8* ip, const u8* udp, u16 n)
{
    u32 sum = 17u + n;
    for (u32 i = 12; i < 20; i += 2) sum += rd16(ip + i);
    for (u32 i = 0; i < n; i += 2)
        sum += ((u16)udp[i] << 8) | ((i + 1 < n) ? udp[i + 1] : 0);
    while (sum >> 16) sum = (sum & 65535u) + (sum >> 16);
    return (u16)~sum;
}
bool udp_send(u32 ip, const u8 mac[6], u16 port, u16 local_port,
              const u8* data, u16 len)
{
    if (len > UDP_MAX_PAYLOAD || !port || !local_port) return false;
    ethernet(mac, 0x0800);
    u8* h = tx + 14;
    h[0] = 0x45; wr16(h + 2, 28 + len); wr16(h + 4, ++ident);
    wr16(h + 6, 0x4000); h[8] = 64; h[9] = 17;
    wr32(h + 12, net_our_ipv4()); wr32(h + 16, ip);
    wr16(h + 10, ipv4_checksum(h, 20));
    u8* u = h + 20;
    wr16(u, local_port); wr16(u + 2, port); wr16(u + 4, 8 + len);
    kmemcpy(u + 8, data, len);
    u16 check = udp_checksum(h, u, 8 + len);
    wr16(u + 6, check ? check : 0xffff);
    return ne2k_send(tx, 42 + len < 60 ? 60 : 42 + len);
}
u16 udp_receive(u32 ip, u16 port, u16 local_port, u8* data, u16 cap)
{
    u16 n = ne2k_recv(rx, sizeof(rx));
    arp_reply(n);
    if (n < 42 || rd16(rx + 12) != 0x0800) return 0;
    const u8* h = rx + 14;
    u16 ihl = (h[0] & 15) * 4, total = rd16(h + 2);
    if ((h[0] >> 4) != 4 || ihl < 20 || total < ihl + 8 || total > n - 14
        || h[9] != 17 || (rd16(h + 6) & 0x3fff)
        || rd32(h + 12) != ip || rd32(h + 16) != net_our_ipv4()
        || ipv4_checksum(h, ihl) != 0) return 0;
    const u8* u = h + ihl;
    u16 size = rd16(u + 4);
    if (rd16(u) != port || rd16(u + 2) != local_port || size < 8
        || size != total - ihl || size - 8 > cap
        || (rd16(u + 6) && udp_checksum(h, u, size) != 0)) return 0;
    kmemcpy(data, u + 8, size - 8);
    return size - 8;
}
