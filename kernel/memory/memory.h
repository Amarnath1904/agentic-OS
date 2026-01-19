#ifndef MEMORY_H
#define MEMORY_H

#include "../kernel.h"

/* ============================================
 * MEMORY MANAGER HEADER
 * Secure memory allocation API
 * ============================================ */

/* Memory protection flags */
#define MEM_READ    0x01
#define MEM_WRITE   0x02
#define MEM_EXEC    0x04

/* Memory allocation flags */
#define MEM_ZERO    0x10    /* Zero memory on allocation */
#define MEM_SECURE  0x20    /* Secure allocation (with canaries) */

/* Memory block header (for bounds checking) */
typedef struct {
    u32 magic;          /* Magic number for validation */
    u32 size;           /* Size of allocated block */
    u32 canary_front;   /* Front canary */
    u32 canary_back;    /* Back canary */
    u8 flags;           /* Protection flags */
} mem_block_header_t;

#define MEM_MAGIC 0xDEADBEEF
#define MEM_CANARY 0xCAFEBABE

/* Function declarations */
void init_memory_manager(void);
void* kmalloc(u32 size, u8 flags);
void kfree(void* ptr);
void* krealloc(void* ptr, u32 new_size);
u32 get_memory_usage(void);
bool validate_pointer(void* ptr);

#endif /* MEMORY_H */

