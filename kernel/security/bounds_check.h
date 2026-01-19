#ifndef BOUNDS_CHECK_H
#define BOUNDS_CHECK_H

#include "../kernel.h"

/* ============================================
 * BOUNDS CHECKING HEADER
 * ============================================ */

bool check_array_bounds(void* array, u32 index, u32 array_size, u32 element_size);
bool validate_pointer_range(void* ptr, u32 size, void* base, u32 range_size);
bool safe_memcpy(void* dest, void* src, u32 size, void* dest_base, u32 dest_size, void* src_base, u32 src_size);
bool safe_strcpy(char* dest, const char* src, u32 dest_size);

#endif /* BOUNDS_CHECK_H */

