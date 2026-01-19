/* ============================================
 * SYSCALL HANDLERS
 * System call implementation with validation
 * ============================================ */

#include "syscall.h"
#include "../memory/memory.h"
#include "../drivers/vga.h"
#include "../kernel.h"

/* ============================================
 * Initialize Syscalls
 * ============================================ */
void init_syscalls(void)
{
    /* Syscall handler will be registered in IDT */
    /* This is a placeholder */
}

/* ============================================
 * Syscall Handler (Dispatcher)
 * ============================================ */
void syscall_handler(syscall_params_t* params)
{
    if (params == NULL) {
        return;
    }
    
    u32 result = 0;
    
    /* Validate syscall number */
    if (params->syscall_num > 7) {
        /* Invalid syscall */
        return;
    }
    
    /* Dispatch to appropriate handler */
    switch (params->syscall_num) {
        case SYS_EXIT:
            result = syscall_exit(params->arg1);
            break;
        case SYS_WRITE:
            result = syscall_write(params->arg1, (const char*)params->arg2, params->arg3);
            break;
        case SYS_READ:
            result = syscall_read(params->arg1, (char*)params->arg2, params->arg3);
            break;
        case SYS_MALLOC:
            result = (u32)syscall_malloc(params->arg1);
            break;
        case SYS_FREE:
            result = syscall_free((void*)params->arg1);
            break;
        case SYS_YIELD:
            /* Yield is handled by scheduler */
            result = 0;
            break;
        default:
            result = STATUS_INVALID;
            break;
    }
    
    /* Return value is stored in EAX by syscall stub */
    asm volatile("mov %0, %%eax" :: "r"(result));
}

/* ============================================
 * Syscall: Exit
 * ============================================ */
u32 syscall_exit(u32 status)
{
    UNUSED(status);
    /* TODO: Clean up task and remove from scheduler */
    return STATUS_OK;
}

/* ============================================
 * Syscall: Write
 * ============================================ */
u32 syscall_write(u32 fd, const char* buf, u32 count)
{
    if (buf == NULL) {
        return STATUS_INVALID;
    }
    
    /* Validate buffer bounds */
    extern bool validate_pointer(void* ptr);
    if (!validate_pointer((void*)buf)) {
        return STATUS_INVALID;
    }
    
    /* For now, only support stdout (fd=1) */
    if (fd != 1) {
        return STATUS_INVALID;
    }
    
    /* Write to VGA */
    for (u32 i = 0; i < count; i++) {
        if (buf[i] == '\0') {
            break;
        }
        vga_putchar(buf[i]);
    }
    
    return count;
}

/* ============================================
 * Syscall: Read
 * ============================================ */
u32 syscall_read(u32 fd, char* buf, u32 count)
{
    UNUSED(fd);
    UNUSED(buf);
    UNUSED(count);
    /* TODO: Implement keyboard input */
    return 0;
}

/* ============================================
 * Syscall: Malloc
 * ============================================ */
u32 syscall_malloc(u32 size)
{
    void* ptr = kmalloc(size, MEM_ZERO | MEM_SECURE);
    return (ptr != NULL) ? (u32)ptr : 0;
}

/* ============================================
 * Syscall: Free
 * ============================================ */
u32 syscall_free(void* ptr)
{
    if (ptr == NULL) {
        return STATUS_INVALID;
    }
    
    kfree(ptr);
    return STATUS_OK;
}

