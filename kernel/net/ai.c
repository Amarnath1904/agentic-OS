#include "ai.h"
#include "udp.h"
#include "timer.h"
#include "net_config.h"
#include "net_endian.h"
#include "../string/kstring.h"

/* AGI1: magic[4], id:u32, offset:u16, total:u16, kind:u8, zero[3].
 * Big endian. Retries reuse IDs; replies are fetched in 512-byte chunks. */
static u8 request[16 + AI_PROMPT_CAP], response[UDP_MAX_PAYLOAD];
static u32 serial;
static u16 rd16(const u8* p) { return ((u16)p[0] << 8) | p[1]; }
static void wr16(u8* p, u16 v) { p[0] = v >> 8; p[1] = v; }

static const char* exchange(const char* prompt, char* reply, bool* remote_error, u8 kind)
{
    u16 prompt_len = kstrlen(prompt), offset = 0, total = 0;
    *remote_error = false; reply[0] = 0;
    if (!prompt_len || prompt_len >= AI_PROMPT_CAP) return "Use a question of 1-511 characters.";
    u32 ip = net_ipv4_from_octets(AI_BRIDGE_A, AI_BRIDGE_B, AI_BRIDGE_C, AI_BRIDGE_D);
    u8 mac[6];
    if (!udp_resolve(ip, mac)) return "Network unavailable: check NE2000 and slirp configuration.";
    u32 lo, hi;
    asm volatile("rdtsc" : "=a"(lo), "=d"(hi));
    u32 id = lo ^ hi ^ ++serial;
    u16 local_port = 49152u + (id & 16383u);
    kmemset(request, 0, 16);
    kmemcpy(request, "AGI1", 4);
    wr16(request + 4, id >> 16); wr16(request + 6, id);
    kmemcpy(request + 16, prompt, prompt_len);
    u32 start = net_ticks(), last = start - 100u;
    while ((u32)(net_ticks() - start) < 9000u) {
        u32 now = net_ticks();
        if ((u32)(now - last) >= 100u) {
            request[12] = offset ? 2 : kind;
            wr16(request + 8, offset);
            if (!udp_send(ip, mac, AI_BRIDGE_PORT, local_port, request,
                          offset ? 16 : 16 + prompt_len))
                return "Network send failed.";
            last = now;
        }
        u16 n = udp_receive(ip, AI_BRIDGE_PORT, local_port, response, sizeof(response));
        if (n < 16) continue;
        bool match = true;
        for (u16 i = 0; i < 8; i++) if (response[i] != request[i]) match = false;
        if (!match || rd16(response + 8) != offset || response[13]
            || response[14] || response[15]) continue;
        if (response[12] == 3) continue;
        if (response[12] != 4 && response[12] != 5) continue;
        u16 advertised = rd16(response + 10), chunk = n - 16;
        if (!advertised || advertised >= AI_REPLY_CAP || chunk == 0 || chunk > 512
            || offset + chunk > advertised || (offset && advertised != total))
            return "Invalid bridge response.";
        if (offset && *remote_error != (response[12] == 5)) return "Invalid bridge response.";
        total = advertised; *remote_error = response[12] == 5;
        for (u16 i = 0; i < chunk; i++) {
            u8 c = response[16 + i];
            reply[offset + i] = (c == '\n' || (c >= 32 && c <= 126)) ? c : '?';
        }
        offset += chunk; reply[offset] = 0;
        if (offset == total) return NULL;
        last = net_ticks() - 100u;
    }
    return "AI timed out. Check that the bridge is running and can reach Gemini.";
}

const char* ai_ask(const char* prompt, char* reply, bool* remote_error)
{
    return exchange(prompt, reply, remote_error, 1);
}
const char* ai_plan(const char* prompt, char* reply, bool* remote_error)
{
    return exchange(prompt, reply, remote_error, 6);
}
