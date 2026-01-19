#ifndef SYSCALL_H
#define SYSCALL_H

#include "../kernel.h"

/* ============================================
 * SYSCALL HEADER
 * System call definitions
 * ============================================ */

/* Syscall numbers */
#define SYS_EXIT    0
#define SYS_WRITE   1
#define SYS_READ    2
#define SYS_OPEN    3
#define SYS_CLOSE   4
#define SYS_MALLOC  5
#define SYS_FREE    6
#define SYS_YIELD   7

/* Syscall parameter structure */
typedef struct {
    u32 syscall_num;
    u32 arg1;
    u32 arg2;
    u32 arg3;
    u32 arg4;
    u32 arg5;
} syscall_params_t;

/* Function declarations */
void init_syscalls(void);
void syscall_handler(syscall_params_t* params);
u32 syscall_exit(u32 status);
u32 syscall_write(u32 fd, const char* buf, u32 count);
u32 syscall_read(u32 fd, char* buf, u32 count);
u32 syscall_malloc(u32 size);
u32 syscall_free(void* ptr);

#endif /* SYSCALL_H */

