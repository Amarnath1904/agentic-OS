#include "tcp_probe.h"
#include "net_config.h"
#include "net_endian.h"
#include "net.h"
#include "ipv4.h"
#include "eth.h"
#include "ne2k.h"
#include "../string/kstring.h"

#define ARP_HTYPE_ETH   1
#define ARP_PTYPE_IPV4  0x0800u
#define ARP_OPER_REQ    1
#define ARP_OPER_REP    2

#define IP_PROTO_TCP    6u

#define TCP_FLAG_FIN    0x01u
#define TCP_FLAG_SYN    0x02u
#define TCP_FLAG_RST    0x04u
#define TCP_FLAG_ACK    0x10u

#define MIN_ETH_FRAME   60u

static u8 s_tx[1600];
static u8 s_rx[1600];
static u16 s_ip_id = 1;

static void mac_copy(u8* d, const u8* s)
{
    kmemcpy(d, s, 6u);
}

static void mac_broadcast(u8* d)
{
    kmemset(d, 0xFF, 6u);
}

static void eth_build(u8* f, const u8* dst, const u8* src, u16 etype, u16* len_io)
{
    mac_copy(f, dst);
    mac_copy(f + 6u, src);
    f[12] = (u8)(etype >> 8);
    f[13] = (u8)(etype & 0xFFu);
    *len_io = ETH_HDR_LEN;
}

static void our_mac(u8* m)
{
    m[0] = NET_MAC_0;
    m[1] = NET_MAC_1;
    m[2] = NET_MAC_2;
    m[3] = NET_MAC_3;
    m[4] = NET_MAC_4;
    m[5] = NET_MAC_5;
}

static void u32_to_be_bytes(u32 v, u8* b)
{
    b[0] = (u8)(v >> 24);
    b[1] = (u8)(v >> 16);
    b[2] = (u8)(v >> 8);
    b[3] = (u8)v;
}

static u32 be_bytes_to_u32(const u8* b)
{
    return ((u32)b[0] << 24) | ((u32)b[1] << 16) | ((u32)b[2] << 8) | (u32)b[3];
}

static bool arp_send_who_has(u32 tip_nbo)
{
    u16 flen = 0;
    u8* f = s_tx;
    u8 smac[6];

    our_mac(smac);
    mac_broadcast(f);
    mac_copy(f + 6u, smac);
    f[12] = 0x08;
    f[13] = 0x06;
    flen = ETH_HDR_LEN;

    {
        u16* p16 = (u16*)(f + flen);
        p16[0] = net_htons(ARP_HTYPE_ETH);
        p16[1] = net_htons(ARP_PTYPE_IPV4);
        f[flen + 4] = 6;
        f[flen + 5] = 4;
        p16[3] = net_htons(ARP_OPER_REQ);
        flen += 8;
    }
    mac_copy(f + flen, smac);
    flen += 6u;
    u32_to_be_bytes(net_our_ipv4(), f + flen);
    flen += 4u;
    kmemset(f + flen, 0, 6u);
    flen += 6u;
    u32_to_be_bytes(tip_nbo, f + flen);
    flen += 4u;

    while (flen < MIN_ETH_FRAME) {
        f[flen++] = 0;
    }
    return ne2k_send(f, flen);
}

static bool arp_parse_reply(const u8* p, u16 len, u32 want_ip_nbo, u8 mac_out[6])
{
    u16 et;
    u16 oper;
    u32 spa;

    if (len < ETH_HDR_LEN + 28u) {
        return false;
    }
    et = ((u16)p[12] << 8) | (u16)p[13];
    if (et != ETHERTYPE_ARP) {
        return false;
    }
    oper = ((u16)p[ETH_HDR_LEN + 6] << 8) | (u16)p[ETH_HDR_LEN + 7];
    if (oper != ARP_OPER_REP) {
        return false;
    }
    spa = be_bytes_to_u32(p + ETH_HDR_LEN + 14u);
    if (spa != want_ip_nbo) {
        return false;
    }
    mac_copy(mac_out, p + ETH_HDR_LEN + 8u);
    return true;
}

static bool arp_resolve(u32 target_ip_nbo, u8 nh_mac[6])
{
    u32 spin;

    if (!arp_send_who_has(target_ip_nbo)) {
        return false;
    }
    for (spin = 0; spin < TCP_PROBE_TIMEOUT_SPINS; spin++) {
        u16 r = ne2k_recv(s_rx, (u16)sizeof(s_rx));
        if (r > 0u && arp_parse_reply(s_rx, r, target_ip_nbo, nh_mac)) {
            return true;
        }
    }
    return false;
}

static u16 tcp_checksum_ipv4(
    u32 src_nbo,
    u32 dst_nbo,
    u16 tcp_len_host,
    const u8* tcp_hdr,
    u16 tcp_total_len)
{
    u32 sum = 0;
    u32 i;
    u16 w;

    sum += (u16)(src_nbo >> 16) + (u16)(src_nbo & 0xFFFFu);
    sum += (u16)(dst_nbo >> 16) + (u16)(dst_nbo & 0xFFFFu);
    sum += (u32)IP_PROTO_TCP;
    sum += (u32)tcp_len_host;

    for (i = 0; i + 1u < (u32)tcp_total_len; i += 2u) {
        w = ((u16)tcp_hdr[i] << 8) | (u16)tcp_hdr[i + 1u];
        sum += w;
    }
    if ((tcp_total_len & 1u) != 0u) {
        sum += (u16)tcp_hdr[tcp_total_len - 1u] << 8;
    }
    while (sum >> 16) {
        sum = (sum & 0xFFFFu) + (sum >> 16);
    }
    return (u16)~sum;
}

static bool send_tcp_syn(
    const u8* eth_dst,
    u32 dst_ip_nbo,
    u16 dst_port_be,
    u16 src_port_be,
    u32 isn)
{
    u16 flen;
    u8* f = s_tx;
    u8 smac[6];
    u8 ip[20];
    u8 tcp[20];
    u16 total_len;
    u16 csum;

    our_mac(smac);
    flen = 0;
    eth_build(f, eth_dst, smac, ETHERTYPE_IPV4, &flen);

    kmemset(ip, 0, sizeof(ip));
    ip[0] = 0x45;
    ip[1] = 0;
    total_len = (u16)(20u + 20u);
    ip[2] = (u8)(total_len >> 8);
    ip[3] = (u8)(total_len & 0xFFu);
    ip[4] = (u8)(s_ip_id >> 8);
    ip[5] = (u8)(s_ip_id & 0xFFu);
    s_ip_id++;
    ip[6] = 0;
    ip[7] = 0;
    ip[8] = 64;
    ip[9] = (u8)IP_PROTO_TCP;
    ip[10] = 0;
    ip[11] = 0;
    u32_to_be_bytes(net_our_ipv4(), ip + 12);
    u32_to_be_bytes(dst_ip_nbo, ip + 16);
    csum = ipv4_checksum(ip, 20u);
    ip[10] = (u8)(csum >> 8);
    ip[11] = (u8)(csum & 0xFFu);

    kmemset(tcp, 0, sizeof(tcp));
    tcp[0] = (u8)(src_port_be >> 8);
    tcp[1] = (u8)(src_port_be & 0xFFu);
    tcp[2] = (u8)(dst_port_be >> 8);
    tcp[3] = (u8)(dst_port_be & 0xFFu);
    tcp[4] = (u8)(isn >> 24);
    tcp[5] = (u8)(isn >> 16);
    tcp[6] = (u8)(isn >> 8);
    tcp[7] = (u8)isn;
    tcp[12] = 0x50;
    tcp[13] = (u8)TCP_FLAG_SYN;
    tcp[14] = 0;
    tcp[15] = 0;

    csum = tcp_checksum_ipv4(
        net_our_ipv4(),
        dst_ip_nbo,
        20u,
        tcp,
        20u);
    tcp[16] = (u8)(csum >> 8);
    tcp[17] = (u8)(csum & 0xFFu);

    kmemcpy(f + flen, ip, 20u);
    flen = (u16)(flen + 20u);
    kmemcpy(f + flen, tcp, 20u);
    flen = (u16)(flen + 20u);

    while (flen < MIN_ETH_FRAME) {
        f[flen++] = 0;
    }
    return ne2k_send(f, flen);
}

static bool send_tcp_rst_ack(
    const u8* eth_dst,
    u32 dst_ip_nbo,
    u16 dst_port_be,
    u16 src_port_be,
    u32 seq,
    u32 ack)
{
    u16 flen;
    u8* f = s_tx;
    u8 smac[6];
    u8 ip[20];
    u8 tcp[20];
    u16 total_len;
    u16 csum;

    our_mac(smac);
    flen = 0;
    eth_build(f, eth_dst, smac, ETHERTYPE_IPV4, &flen);

    kmemset(ip, 0, sizeof(ip));
    ip[0] = 0x45;
    ip[1] = 0;
    total_len = (u16)(20u + 20u);
    ip[2] = (u8)(total_len >> 8);
    ip[3] = (u8)(total_len & 0xFFu);
    ip[4] = (u8)(s_ip_id >> 8);
    ip[5] = (u8)(s_ip_id & 0xFFu);
    s_ip_id++;
    ip[6] = 0;
    ip[7] = 0;
    ip[8] = 64;
    ip[9] = (u8)IP_PROTO_TCP;
    ip[10] = 0;
    ip[11] = 0;
    u32_to_be_bytes(net_our_ipv4(), ip + 12);
    u32_to_be_bytes(dst_ip_nbo, ip + 16);
    csum = ipv4_checksum(ip, 20u);
    ip[10] = (u8)(csum >> 8);
    ip[11] = (u8)(csum & 0xFFu);

    kmemset(tcp, 0, sizeof(tcp));
    tcp[0] = (u8)(src_port_be >> 8);
    tcp[1] = (u8)(src_port_be & 0xFFu);
    tcp[2] = (u8)(dst_port_be >> 8);
    tcp[3] = (u8)(dst_port_be & 0xFFu);
    tcp[4] = (u8)(seq >> 24);
    tcp[5] = (u8)(seq >> 16);
    tcp[6] = (u8)(seq >> 8);
    tcp[7] = (u8)seq;
    tcp[8] = (u8)(ack >> 24);
    tcp[9] = (u8)(ack >> 16);
    tcp[10] = (u8)(ack >> 8);
    tcp[11] = (u8)ack;
    tcp[12] = 0x50;
    tcp[13] = (u8)(TCP_FLAG_RST | TCP_FLAG_ACK);
    tcp[14] = 0;
    tcp[15] = 0;

    csum = tcp_checksum_ipv4(
        net_our_ipv4(),
        dst_ip_nbo,
        20u,
        tcp,
        20u);
    tcp[16] = (u8)(csum >> 8);
    tcp[17] = (u8)(csum & 0xFFu);

    kmemcpy(f + flen, ip, 20u);
    flen = (u16)(flen + 20u);
    kmemcpy(f + flen, tcp, 20u);
    flen = (u16)(flen + 20u);

    while (flen < MIN_ETH_FRAME) {
        f[flen++] = 0;
    }
    return ne2k_send(f, flen);
}

static bool parse_synack(
    const u8* p,
    u16 len,
    u32 dst_ip_nbo,
    u16 src_port_be,
    u16 dst_port_be,
    u32* srv_seq_out,
    u32* ack_to_us_out)
{
    u16 et;
    const u8* ip;
    u8 ihl;
    const u8* tcp;
    u16 sport;
    u16 dport;
    u8 off;
    u8 flg;

    if (len < ETH_HDR_LEN + 20u + 20u) {
        return false;
    }
    et = ((u16)p[12] << 8) | (u16)p[13];
    if (et != ETHERTYPE_IPV4) {
        return false;
    }
    ip = p + ETH_HDR_LEN;
    if ((ip[0] >> 4) != 4u) {
        return false;
    }
    ihl = (u8)((ip[0] & 0x0Fu) * 4u);
    if (ihl < 20u || (u16)(ETH_HDR_LEN + ihl + 20u) > len) {
        return false;
    }
    if (ip[9] != IP_PROTO_TCP) {
        return false;
    }
    if (be_bytes_to_u32(ip + 12) != dst_ip_nbo) {
        return false;
    }
    if (be_bytes_to_u32(ip + 16) != net_our_ipv4()) {
        return false;
    }
    tcp = ip + ihl;
    sport = ((u16)tcp[0] << 8) | (u16)tcp[1];
    dport = ((u16)tcp[2] << 8) | (u16)tcp[3];
    if (sport != dst_port_be || dport != src_port_be) {
        return false;
    }
    off = (u8)((tcp[12] >> 4) * 4u);
    if (off < 20u || (u16)(tcp - p + off) > len) {
        return false;
    }
    flg = tcp[13];
    if ((flg & (TCP_FLAG_SYN | TCP_FLAG_ACK)) != (TCP_FLAG_SYN | TCP_FLAG_ACK)) {
        return false;
    }
    *srv_seq_out = be_bytes_to_u32(tcp + 4);
    *ack_to_us_out = be_bytes_to_u32(tcp + 8);
    return true;
}

bool tcp_probe_host_port(u32 dst_ip_nbo, u16 dst_port_host)
{
    u32 mask = net_netmask();
    u32 ours = net_our_ipv4();
    u32 gw = net_gw_ipv4();
    u32 nh_ip = dst_ip_nbo;
    u8 nh_mac[6];
    /* Builders serialize these numeric values explicitly, so do not swap. */
    u16 dst_port_be = dst_port_host;
    u16 src_port_be = 0xC000u;
    u32 isn = 0x12345678u;
    u32 spin;
    u32 srv_seq;
    u32 ack_to_us;

    if ((dst_ip_nbo & mask) != (ours & mask)) {
        nh_ip = gw;
    }
    if (!arp_resolve(nh_ip, nh_mac)) {
        return false;
    }
    if (!send_tcp_syn(nh_mac, dst_ip_nbo, dst_port_be, src_port_be, isn)) {
        return false;
    }
    for (spin = 0; spin < TCP_PROBE_TIMEOUT_SPINS; spin++) {
        u16 r = ne2k_recv(s_rx, (u16)sizeof(s_rx));
        if (r > 0u
            && parse_synack(
                s_rx,
                r,
                dst_ip_nbo,
                src_port_be,
                dst_port_be,
                &srv_seq,
                &ack_to_us)) {
            if (ack_to_us != isn + 1u) {
                continue;
            }
            (void)send_tcp_rst_ack(
                nh_mac,
                dst_ip_nbo,
                dst_port_be,
                src_port_be,
                isn + 1u,
                srv_seq + 1u);
            return true;
        }
    }
    return false;
}
