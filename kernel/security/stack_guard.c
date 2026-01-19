/* ============================================
 * STACK GUARD
 * Stack canary implementation for overflow detection
 * ============================================ */

#include "stack_guard.h"
#include "../kernel.h"

#define CANARY_VALUE 0xCAFEBABE

/* ============================================
 * Generate Stack Canary
 * ============================================ */
u32 generate_canary(void)
{
    /* For now, use fixed canary */
    /* TODO: Use random number generator */
    return CANARY_VALUE;
}

/* ============================================
 * Set Stack Canary
 * ============================================ */
void set_stack_canary(u32* canary_ptr)
{
    if (canary_ptr != NULL) {
        *canary_ptr = generate_canary();
    }
}

/* ============================================
 * Check Stack Canary
 * ============================================ */
bool check_stack_canary(u32 canary_value)
{
    return (canary_value == CANARY_VALUE);
}

/* ============================================
 * Stack Overflow Detected
 * ============================================ */
void stack_overflow_detected(void)
{
    panic("Stack overflow detected! Stack canary corrupted!");
}

