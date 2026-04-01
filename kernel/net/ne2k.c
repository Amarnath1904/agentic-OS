/* ISA NE2000 (DP8390) — polling RX/TX for Bochs vnet */

#include "ne2k.h"
#include "net_config.h"
#include "../io.h"
#include "../kernel.h"

#define NE_DATA    0x10
#define NE_RESET   0x1F
#define NE_CR      0x00
#define NE_PSTART  0x01
#define NE_PSTOP   0x02
#define NE_BNRY    0x03
#define NE_TSR     0x04
#define NE_TPSR    0x04
#define NE_TBCR0   0x05
#define NE_TBCR1   0x06
#define NE_ISR     0x07
#define NE_RSAR0   0x08
#define NE_RSAR1   0x09
#define NE_RBCR0   0x0A
#define NE_RBCR1   0x0B

#define NE_CR_STOP    0x01
#define NE_CR_START   0x02
#define NE_CR_TXP     0x04
#define NE_CR_RD0     0x08
#define NE_CR_RD1     0x10
#define NE_CR_RD2     0x20
#define NE_CR_PG0     0x00
#define NE_CR_PG1     0x40
#define NE_CR_PG2     0x80

#define NE_TX_PAGE   0x40
#define NE_RX_START  0x4C
#define NE_RX_END    0x80

#define NE_ISR_PRX    0x01
#define NE_ISR_PTX    0x02
#define NE_ISR_RDC    0x40

#define BASE ((u16)NE2K_IO_BASE)

static void ne_stall(void)
{
    for (volatile u32 i = 0; i < 5000u; i++) {
    }
}

static void ne_cmd(u8 v)
{
    outb(BASE + NE_CR, v);
}

static bool ne_wait_rdc(void)
{
    for (u32 n = 0; n < 200000u; n++) {
        if (inb(BASE + NE_ISR) & NE_ISR_RDC) {
            outb(BASE + NE_ISR, NE_ISR_RDC);
            return true;
        }
    }
    return false;
}

static bool ne_wait_tx_done(void)
{
    for (u32 n = 0; n < 500000u; n++) {
        u8 isr = inb(BASE + NE_ISR);
        if (isr & NE_ISR_PTX) {
            outb(BASE + NE_ISR, NE_ISR_PTX);
            return true;
        }
    }
    return false;
}

bool ne2k_init(void)
{
    u32 i;

    inb(BASE + NE_RESET);
    inb(BASE + NE_RESET);
    ne_stall();

    ne_cmd(NE_CR_PG0 | NE_CR_STOP);
    outb(BASE + NE_ISR, 0xFF);
    outb(BASE + NE_RBCR0, 0);
    outb(BASE + NE_RBCR1, 0);
    outb(BASE + 0x0C, 0x04);
    outb(BASE + 0x0D, 0x00);
    outb(BASE + 0x0E, 0x49);
    outb(BASE + 0x0F, 0x00);

    for (i = 0; i < 8u; i++) {
        outb(BASE + 8u + i, 0xFF);
    }

    outb(BASE + NE_PSTART, NE_RX_START);
    outb(BASE + NE_PSTOP, NE_RX_END);
    outb(BASE + NE_BNRY, NE_RX_START);

    ne_cmd(NE_CR_PG1 | NE_CR_STOP);
    outb(BASE + 0x01, NET_MAC_0);
    outb(BASE + 0x02, NET_MAC_1);
    outb(BASE + 0x03, NET_MAC_2);
    outb(BASE + 0x04, NET_MAC_3);
    outb(BASE + 0x05, NET_MAC_4);
    outb(BASE + 0x06, NET_MAC_5);
    outb(BASE + 0x07, (u8)(NE_RX_START + 1));

    ne_cmd(NE_CR_PG0 | NE_CR_STOP);
    outb(BASE + NE_ISR, 0xFF);
    ne_cmd(NE_CR_PG0 | NE_CR_START);

    return true;
}

bool ne2k_send(const u8* data, u16 len)
{
    u32 i;

    if (len > 1514u) {
        return false;
    }

    ne_cmd(NE_CR_PG0 | NE_CR_STOP);
    outb(BASE + NE_RBCR0, (u8)(len & 0xFF));
    outb(BASE + NE_RBCR1, (u8)(len >> 8));
    outb(BASE + NE_RSAR0, 0x00);
    outb(BASE + NE_RSAR1, NE_TX_PAGE);
    ne_cmd(NE_CR_PG0 | NE_CR_RD1 | NE_CR_START);

    for (i = 0; i < (u32)len; i++) {
        outb(BASE + NE_DATA, data[i]);
    }

    if (!ne_wait_rdc()) {
        ne_cmd(NE_CR_PG0 | NE_CR_START);
        return false;
    }

    outb(BASE + NE_TBCR0, (u8)(len & 0xFF));
    outb(BASE + NE_TBCR1, (u8)(len >> 8));
    outb(BASE + NE_TPSR, NE_TX_PAGE);
    ne_cmd(NE_CR_PG0 | NE_CR_TXP | NE_CR_START);

    if (!ne_wait_tx_done()) {
        return false;
    }
    return true;
}

u16 ne2k_recv(u8* buf, u16 max_len)
{
    u8 bnry;
    u8 curr;
    u8 next;
    u16 ring_addr;
    u8 hdr[4];
    u16 plen;
    u32 i;

    if (!(inb(BASE + NE_ISR) & NE_ISR_PRX)) {
        return 0;
    }

    ne_cmd(NE_CR_PG0 | NE_CR_STOP);
    bnry = inb(BASE + NE_BNRY);
    ne_cmd(NE_CR_PG1 | NE_CR_STOP);
    curr = inb(BASE + 0x07);
    ne_cmd(NE_CR_PG0 | NE_CR_STOP);

    next = (u8)(bnry + 1);
    if (next >= NE_RX_END) {
        next = NE_RX_START;
    }
    if (next == curr) {
        outb(BASE + NE_ISR, NE_ISR_PRX);
        ne_cmd(NE_CR_PG0 | NE_CR_START);
        return 0;
    }

    ring_addr = (u16)((u16)next << 8);
    outb(BASE + NE_RBCR0, 4);
    outb(BASE + NE_RBCR1, 0);
    outb(BASE + NE_RSAR0, (u8)(ring_addr & 0xFF));
    outb(BASE + NE_RSAR1, (u8)(ring_addr >> 8));
    ne_cmd(NE_CR_PG0 | NE_CR_RD0 | NE_CR_START);

    for (i = 0; i < 4u; i++) {
        hdr[i] = inb(BASE + NE_DATA);
    }
    if (!ne_wait_rdc()) {
        ne_cmd(NE_CR_PG0 | NE_CR_START);
        return 0;
    }

    next = hdr[1];
    plen = (u16)hdr[2] | ((u16)hdr[3] << 8);
    if (plen <= 4u) {
        outb(BASE + NE_ISR, NE_ISR_PRX);
        ne_cmd(NE_CR_PG0 | NE_CR_START);
        return 0;
    }
    /* plen = ethernet frame + FCS; skip 4-byte NIC header in ring */
    plen = (u16)(plen - 4u);
    if (plen > max_len) {
        plen = max_len;
    }

    ring_addr = (u16)(ring_addr + 4u);

    outb(BASE + NE_RBCR0, (u8)(plen & 0xFF));
    outb(BASE + NE_RBCR1, (u8)(plen >> 8));
    outb(BASE + NE_RSAR0, (u8)(ring_addr & 0xFF));
    outb(BASE + NE_RSAR1, (u8)(ring_addr >> 8));
    ne_cmd(NE_CR_PG0 | NE_CR_RD0 | NE_CR_START);

    for (i = 0; i < (u32)plen; i++) {
        buf[i] = inb(BASE + NE_DATA);
    }
    if (!ne_wait_rdc()) {
        ne_cmd(NE_CR_PG0 | NE_CR_START);
        return 0;
    }

    {
        u8 nxt = hdr[1];
        u8 nb = (u8)(nxt - 1u);
        if (nb < NE_RX_START) {
            nb = (u8)(NE_RX_END - 1u);
        }
        outb(BASE + NE_BNRY, nb);
    }

    outb(BASE + NE_ISR, NE_ISR_PRX);
    ne_cmd(NE_CR_PG0 | NE_CR_START);
    return plen;
}
