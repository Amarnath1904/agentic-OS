#include "ipv4.h"
#include "net_endian.h"

static const char* skip_leading_ws(const char* s)
{
    while (*s == ' ' || *s == '\t') {
        s++;
    }
    return s;
}

u16 ipv4_checksum(const void* hdr, u32 ihl_bytes)
{
    const u8* b = (const u8*)hdr;
    u32 sum = 0;

    while (ihl_bytes > 1u) {
        sum += ((u32)b[0] << 8) | (u32)b[1];
        b += 2;
        ihl_bytes -= 2u;
    }
    if (ihl_bytes != 0u) {
        sum += (u32)b[0] << 8;
    }
    while (sum >> 16) {
        sum = (sum & 0xFFFFu) + (sum >> 16);
    }
    return (u16)(~sum & 0xFFFFu);
}

static int dec_octet(const char** pp)
{
    int v = 0;
    int d = 0;

    while (**pp >= '0' && **pp <= '9') {
        v = v * 10 + (**pp - '0');
        d++;
        (*pp)++;
        if (d > 3) {
            return -1;
        }
    }
    if (d == 0) {
        return -1;
    }
    if (v > 255) {
        return -1;
    }
    return v;
}

bool ipv4_parse_dotted(const char* s, u32* out_ipv4_nbo)
{
    int a;
    int b;
    int c;
    int d;

    if (s == NULL || out_ipv4_nbo == NULL) {
        return false;
    }
    s = skip_leading_ws(s);
    a = dec_octet(&s);
    if (a < 0 || *s != '.') {
        return false;
    }
    s++;
    b = dec_octet(&s);
    if (b < 0 || *s != '.') {
        return false;
    }
    s++;
    c = dec_octet(&s);
    if (c < 0 || *s != '.') {
        return false;
    }
    s++;
    d = dec_octet(&s);
    if (d < 0) {
        return false;
    }
    if (*s != '\0' && *s != ' ' && *s != '\t') {
        return false;
    }
    *out_ipv4_nbo = net_ipv4_from_octets((u8)a, (u8)b, (u8)c, (u8)d);
    return true;
}
