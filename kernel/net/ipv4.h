#ifndef IPV4_H
#define IPV4_H

#include "../kernel.h"

u16 ipv4_checksum(const void* hdr, u32 ihl_bytes);
bool ipv4_parse_dotted(const char* s, u32* out_ipv4_nbo);

#endif
