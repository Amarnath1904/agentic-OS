/* ISA DP8390/NE2000, byte remote DMA, polling receive ring. */
#include "ne2k.h"
#include "net_config.h"
#include "../io.h"
#define BASE ((u16)NE2K_IO_BASE)
#define TX_PAGE 0x40
#define RX_START 0x4c
#define RX_END 0x80
static bool ready;
static void reg(u8 r, u8 v) { outb(BASE + r, v); }
static u8 get(u8 r) { return inb(BASE + r); }
static bool wait_bit(u8 mask)
{
    for (u32 i = 0; i < 200000u; i++)
        if (get(7) & mask) return true;
    return false;
}
static bool dma_read(u16 addr, u8* buf, u16 len)
{
    reg(0, 0x22); reg(7, 0x40);
    reg(10, len); reg(11, len >> 8);
    reg(8, addr); reg(9, addr >> 8); reg(0, 0x0a);
    for (u16 i = 0; i < len; i++) buf[i] = get(0x10);
    bool ok = wait_bit(0x40);
    reg(7, 0x40); reg(0, 0x22);
    return ok;
}
bool ne2k_init(void)
{
    ready = false;
    u8 reset = get(0x1f);
    if (reset == 0xff && get(0) == 0xff) return false;
    reg(0x1f, reset);
    if (!wait_bit(0x80)) return false;
    reg(0, 0x21);
    reg(14, 0x48); /* Byte transfers: matches inb/outb on the data port. */
    reg(10, 0); reg(11, 0); reg(12, 0x20); reg(13, 2);
    reg(1, RX_START); reg(2, RX_END); reg(3, RX_START);
    reg(4, TX_PAGE); reg(7, 0xff); reg(15, 0);
    reg(0, 0x61);
    const u8 mac[6] = {NET_MAC_0,NET_MAC_1,NET_MAC_2,NET_MAC_3,NET_MAC_4,NET_MAC_5};
    for (u8 i = 0; i < 6; i++) reg(1 + i, mac[i]);
    reg(7, RX_START + 1);
    for (u8 i = 8; i < 16; i++) reg(i, 0);
    reg(0, 0x22); reg(13, 0); reg(12, 4); reg(7, 0xff);
    ready = true;
    return true;
}
bool ne2k_send(const u8* data, u16 len)
{
    if (!ready || len < 60 || len > 1514) return false;
    reg(0, 0x22); reg(7, 0x4a);
    reg(10, len); reg(11, len >> 8);
    reg(8, 0); reg(9, TX_PAGE); reg(0, 0x12);
    for (u16 i = 0; i < len; i++) reg(0x10, data[i]);
    if (!wait_bit(0x40)) { reg(0, 0x22); return false; }
    reg(7, 0x40); reg(0, 0x22);
    reg(4, TX_PAGE); reg(5, len); reg(6, len >> 8); reg(0, 0x26);
    bool done = wait_bit(0x0a);
    bool ok = done && (get(7) & 2);
    reg(7, 0x0a);
    return ok;
}
u16 ne2k_recv(u8* buf, u16 max_len)
{
    if (!ready) return 0;
    reg(0, 0x22);
    if (get(7) & 0x10) { (void)ne2k_init(); return 0; }
    u8 next = get(3) + 1;
    if (next >= RX_END) next = RX_START;
    reg(0, 0x62);
    u8 curr = get(7);
    reg(0, 0x22);
    /* PRX can be cleared while more than one frame remains in the ring. */
    if (next == curr) return 0;
    u16 addr = (u16)next << 8;
    u8 hdr[4];
    if (!dma_read(addr, hdr, 4)) return 0;
    u16 count = (u16)hdr[2] | ((u16)hdr[3] << 8);
    if (hdr[1] < RX_START || hdr[1] >= RX_END || count < 64 || count > 1518) {
        (void)ne2k_init(); return 0;
    }
    u16 len = count - 4;
    bool valid = (hdr[0] & 1) && len <= max_len;
    if (valid) {
        addr += 4;
        u16 first = ((u16)RX_END << 8) - addr;
        if (first > len) first = len;
        valid = dma_read(addr, buf, first);
        if (valid && first < len)
            valid = dma_read((u16)RX_START << 8, buf + first, len - first);
    }
    reg(3, hdr[1] == RX_START ? RX_END - 1 : hdr[1] - 1);
    reg(7, 1);
    return valid ? len : 0;
}
