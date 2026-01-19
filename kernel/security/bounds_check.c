/* ============================================
 * BOUNDS CHECKING
 * Array and pointer bounds validation
 * ============================================ */

#include "bounds_check.h"
#include "../kernel.h"

/* ============================================
 * Check Array Bounds
 * ============================================ */
bool check_array_bounds(void* array, u32 index, u32 array_size, u32 element_size)
{
    if (array == NULL) {
        return false;
    }
    
    /* Calculate offset */
    u32 offset = index * element_size;
    
    /* Check bounds */
    if (offset >= array_size) {
        return false;
    }
    
    /* Check for overflow */
    if (offset + element_size > array_size) {
        return false;
    }
    
    return true;
}

/* ============================================
 * Validate Pointer Range
 * ============================================ */
bool validate_pointer_range(void* ptr, u32 size, void* base, u32 range_size)
{
    if (ptr == NULL || base == NULL) {
        return false;
    }
    
    u32 ptr_addr = (u32)ptr;
    u32 base_addr = (u32)base;
    
    /* Check if pointer is within range */
    if (ptr_addr < base_addr) {
        return false;
    }
    
    if (ptr_addr + size > base_addr + range_size) {
        return false;
    }
    
    return true;
}

/* ============================================
 * Safe Memory Copy
 * ============================================ */
bool safe_memcpy(void* dest, void* src, u32 size, void* dest_base, u32 dest_size, void* src_base, u32 src_size)
{
    if (dest == NULL || src == NULL) {
        return false;
    }
    
    /* Validate destination range */
    if (!validate_pointer_range(dest, size, dest_base, dest_size)) {
        return false;
    }
    
    /* Validate source range */
    if (!validate_pointer_range(src, size, src_base, src_size)) {
        return false;
    }
    
    /* Perform copy */
    u8* d = (u8*)dest;
    u8* s = (u8*)src;
    for (u32 i = 0; i < size; i++) {
        d[i] = s[i];
    }
    
    return true;
}

/* ============================================
 * Safe String Copy
 * ============================================ */
bool safe_strcpy(char* dest, const char* src, u32 dest_size)
{
    if (dest == NULL || src == NULL) {
        return false;
    }
    
    u32 i = 0;
    while (i < dest_size - 1 && src[i] != '\0') {
        dest[i] = src[i];
        i++;
    }
    
    dest[i] = '\0';
    
    /* Check if source was truncated */
    return (src[i] == '\0');
}

