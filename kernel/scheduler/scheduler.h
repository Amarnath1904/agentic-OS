#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "../kernel.h"

/* ============================================
 * SCHEDULER HEADER
 * Task scheduling and context switching
 * ============================================ */

/* Task states */
#define TASK_RUNNING  0
#define TASK_READY    1
#define TASK_BLOCKED  2
#define TASK_ZOMBIE   3

/* Task priorities */
#define PRIORITY_LOW     0
#define PRIORITY_NORMAL  1
#define PRIORITY_HIGH    2
#define PRIORITY_REALTIME 3

/* Task structure */
typedef struct task {
    u32 pid;                /* Process ID */
    u32 state;              /* Task state */
    u32 priority;           /* Task priority */
    u32 time_slice;         /* Remaining time slice */
    u32* stack;              /* Stack pointer */
    u32* stack_base;         /* Stack base */
    struct task* next;       /* Next task in queue */
} task_t;

/* Function declarations */
void init_scheduler(void);
void scheduler_tick(void);
task_t* create_task(void (*entry)(void), u32 priority);
void destroy_task(u32 pid);
void yield(void);
task_t* get_current_task(void);

#endif /* SCHEDULER_H */

