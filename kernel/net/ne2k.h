#ifndef NE2K_H
#define NE2K_H

#include "../kernel.h"

bool ne2k_init(void);
bool ne2k_send(const u8* data, u16 len);
/* Copies one frame if available; returns frame length or 0 */
u16 ne2k_recv(u8* buf, u16 max_len);

#endif
