/* ============================================
 * SCHEDULER
 * Round-robin task scheduler
 * ============================================ */

#include "scheduler.h"
#include "../memory/memory.h"
#include "../kernel.h"

#define MAX_TASKS 64
#define STACK_SIZE 4096
#define TIME_SLICE 10

static task_t* task_queue = NULL;
static task_t* current_task = NULL;
static u32 next_pid = 1;
static u32 task_count = 0;

/* ============================================
 * Initialize Scheduler
 * ============================================ */
void init_scheduler(void)
{
    task_queue = NULL;
    current_task = NULL;
    next_pid = 1;
    task_count = 0;
}

/* ============================================
 * Create Task
 * ============================================ */
task_t* create_task(void (*entry)(void), u32 priority)
{
    if (task_count >= MAX_TASKS) {
        return NULL;
    }
    
    /* Allocate task structure */
    task_t* task = (task_t*)kmalloc(sizeof(task_t), MEM_ZERO);
    if (task == NULL) {
        return NULL;
    }
    
    /* Allocate stack */
    task->stack_base = (u32*)kmalloc(STACK_SIZE, MEM_ZERO);
    if (task->stack_base == NULL) {
        kfree(task);
        return NULL;
    }
    
    /* Initialize task */
    task->pid = next_pid++;
    task->state = TASK_READY;
    task->priority = priority;
    task->time_slice = TIME_SLICE;
    task->stack = task->stack_base + (STACK_SIZE / sizeof(u32)) - 1;
    
    /* Set up stack for task entry */
    task->stack[0] = 0x202;  /* EFLAGS */
    task->stack[-1] = 0x08;  /* CS */
    task->stack[-2] = (u32)entry;  /* EIP */
    task->stack -= 3;
    
    /* Add to queue */
    task->next = task_queue;
    task_queue = task;
    
    task_count++;
    
    return task;
}

/* ============================================
 * Destroy Task
 * ============================================ */
void destroy_task(u32 pid)
{
    task_t* prev = NULL;
    task_t* curr = task_queue;
    
    while (curr != NULL) {
        if (curr->pid == pid) {
            /* Remove from queue */
            if (prev == NULL) {
                task_queue = curr->next;
            } else {
                prev->next = curr->next;
            }
            
            /* Free resources */
            if (curr->stack_base != NULL) {
                kfree(curr->stack_base);
            }
            kfree(curr);
            
            task_count--;
            return;
        }
        
        prev = curr;
        curr = curr->next;
    }
}

/* ============================================
 * Scheduler Tick
 * ============================================ */
void scheduler_tick(void)
{
    if (task_queue == NULL) {
        return;
    }
    
    if (current_task == NULL) {
        /* Start first task */
        current_task = task_queue;
        current_task->state = TASK_RUNNING;
        return;
    }
    
    /* Decrement time slice */
    current_task->time_slice--;
    
    if (current_task->time_slice <= 0) {
        /* Time slice expired, switch task */
        current_task->state = TASK_READY;
        current_task->time_slice = TIME_SLICE;
        
        /* Find next task */
        task_t* next = current_task->next;
        if (next == NULL) {
            next = task_queue;
        }
        
        current_task = next;
        current_task->state = TASK_RUNNING;
    }
}

/* ============================================
 * Yield CPU
 * ============================================ */
void yield(void)
{
    if (current_task != NULL) {
        current_task->time_slice = 0;
        scheduler_tick();
    }
}

/* ============================================
 * Get Current Task
 * ============================================ */
task_t* get_current_task(void)
{
    return current_task;
}

