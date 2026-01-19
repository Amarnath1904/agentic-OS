/* ============================================
 * MEMORY MANAGER
 * Secure heap allocator with bounds checking
 * ============================================ */

#include "memory.h"
#include "../kernel.h"

/* Heap boundaries */
#define HEAP_START 0x400000  /* 4MB */
#define HEAP_END   0x800000  /* 8MB */
#define HEAP_SIZE  (HEAP_END - HEAP_START)

static u32 heap_ptr = HEAP_START;
static u32 memory_used = 0;

/* ============================================
 * Initialize Memory Manager
 * ============================================ */
void init_memory_manager(void)
{
    heap_ptr = HEAP_START;
    memory_used = 0;
    
    /* Zero out heap */
    u8* heap = (u8*)HEAP_START;
    for (u32 i = 0; i < HEAP_SIZE; i++) {
        heap[i] = 0;
    }
}

/* ============================================
 * Allocate Memory (with bounds checking)
 * ============================================ */
void* kmalloc(u32 size, u8 flags)
{
    if (size == 0) {
        return NULL;
    }
    
    /* Calculate total size including header and canaries */
    u32 header_size = sizeof(mem_block_header_t);
    u32 canary_size = sizeof(u32) * 2;  /* Front and back canaries */
    u32 total_size = header_size + size + canary_size;
    
    /* Align to 4 bytes */
    total_size = (total_size + 3) & ~3;
    
    /* Check if we have enough memory */
    if (heap_ptr + total_size > HEAP_END) {
        panic("Out of memory!");
        return NULL;
    }
    
    /* Allocate block */
    mem_block_header_t* header = (mem_block_header_t*)heap_ptr;
    heap_ptr += total_size;
    
    /* Initialize header */
    header->magic = MEM_MAGIC;
    header->size = size;
    header->flags = flags;
    header->canary_front = MEM_CANARY;
    header->canary_back = MEM_CANARY;
    
    /* Get pointer to user data */
    void* ptr = (u8*)header + header_size;
    
    /* Zero memory if requested */
    if (flags & MEM_ZERO) {
        u8* data = (u8*)ptr;
        for (u32 i = 0; i < size; i++) {
            data[i] = 0;
        }
    }
    
    /* Set back canary */
    u32* back_canary = (u32*)((u8*)ptr + size);
    *back_canary = MEM_CANARY;
    
    memory_used += total_size;
    
    return ptr;
}

/* ============================================
 * Free Memory (with validation)
 * ============================================ */
void kfree(void* ptr)
{
    if (ptr == NULL) {
        return;
    }
    
    /* Validate pointer */
    if (!validate_pointer(ptr)) {
        panic("Invalid pointer passed to kfree!");
        return;
    }
    
    /* Get header */
    mem_block_header_t* header = (mem_block_header_t*)((u8*)ptr - sizeof(mem_block_header_t));
    
    /* Check canaries */
    if (header->canary_front != MEM_CANARY) {
        panic("Front canary corrupted! Buffer underflow detected!");
    }
    
    u32* back_canary = (u32*)((u8*)ptr + header->size);
    if (*back_canary != MEM_CANARY) {
        panic("Back canary corrupted! Buffer overflow detected!");
    }
    
    /* Zero memory for security */
    u8* data = (u8*)ptr;
    for (u32 i = 0; i < header->size; i++) {
        data[i] = 0;
    }
    
    /* Invalidate header */
    header->magic = 0;
    header->size = 0;
}

/* ============================================
 * Reallocate Memory
 * ============================================ */
void* krealloc(void* ptr, u32 new_size)
{
    if (ptr == NULL) {
        return kmalloc(new_size, MEM_ZERO);
    }
    
    if (new_size == 0) {
        kfree(ptr);
        return NULL;
    }
    
    /* Validate pointer */
    if (!validate_pointer(ptr)) {
        panic("Invalid pointer passed to krealloc!");
        return NULL;
    }
    
    /* Get old size */
    mem_block_header_t* header = (mem_block_header_t*)((u8*)ptr - sizeof(mem_block_header_t));
    u32 old_size = header->size;
    
    /* Allocate new block */
    void* new_ptr = kmalloc(new_size, header->flags);
    if (new_ptr == NULL) {
        return NULL;
    }
    
    /* Copy data */
    u32 copy_size = (old_size < new_size) ? old_size : new_size;
    u8* src = (u8*)ptr;
    u8* dst = (u8*)new_ptr;
    for (u32 i = 0; i < copy_size; i++) {
        dst[i] = src[i];
    }
    
    /* Free old block */
    kfree(ptr);
    
    return new_ptr;
}

/* ============================================
 * Validate Pointer
 * ============================================ */
bool validate_pointer(void* ptr)
{
    if (ptr == NULL) {
        return false;
    }
    
    /* Check if pointer is in heap range */
    if ((u32)ptr < HEAP_START || (u32)ptr >= HEAP_END) {
        return false;
    }
    
    /* Get header */
    mem_block_header_t* header = (mem_block_header_t*)((u8*)ptr - sizeof(mem_block_header_t));
    
    /* Check magic number */
    if (header->magic != MEM_MAGIC) {
        return false;
    }
    
    return true;
}

/* ============================================
 * Get Memory Usage
 * ============================================ */
u32 get_memory_usage(void)
{
    return memory_used;
}

