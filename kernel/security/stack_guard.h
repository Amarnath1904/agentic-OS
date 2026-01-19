#ifndef STACK_GUARD_H
#define STACK_GUARD_H

#include "../kernel.h"

/* ============================================
 * STACK GUARD HEADER
 * ============================================ */

u32 generate_canary(void);
void set_stack_canary(u32* canary_ptr);
bool check_stack_canary(u32 canary_value);
void stack_overflow_detected(void);

#endif /* STACK_GUARD_H */

