#ifndef KERNEL_H
#define KERNEL_H

/* ============================================
 * KERNEL HEADER
 * Common types, constants, and declarations
 * ============================================ */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* ============================================
 * Type Definitions
 * ============================================ */
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;

/* ============================================
 * Constants
 * ============================================ */
#ifndef NULL
#define NULL ((void*)0)
#endif
#define UNUSED(x) ((void)(x))

/* Memory addresses */
#define KERNEL_BASE    0x10000
#define KERNEL_STACK   0x90000
#define VIDEO_MEMORY   0xB8000

/* Screen dimensions */
#define SCREEN_WIDTH   80
#define SCREEN_HEIGHT  25
#define SCREEN_SIZE    (SCREEN_WIDTH * SCREEN_HEIGHT)

/* Colors */
#define COLOR_BLACK         0x0
#define COLOR_BLUE          0x1
#define COLOR_GREEN         0x2
#define COLOR_CYAN          0x3
#define COLOR_RED           0x4
#define COLOR_MAGENTA       0x5
#define COLOR_BROWN         0x6
#define COLOR_LIGHT_GRAY    0x7
#define COLOR_DARK_GRAY     0x8
#define COLOR_LIGHT_BLUE    0x9
#define COLOR_LIGHT_GREEN   0xA
#define COLOR_LIGHT_CYAN    0xB
#define COLOR_LIGHT_RED     0xC
#define COLOR_LIGHT_MAGENTA 0xD
#define COLOR_YELLOW        0xE
#define COLOR_WHITE         0xF

/* Status codes */
#define STATUS_OK           0
#define STATUS_ERROR        -1
#define STATUS_INVALID      -2
#define STATUS_NOT_FOUND    -3
#define STATUS_OUT_OF_MEM   -4

/* ============================================
 * Function Declarations
 * ============================================ */

/* Kernel main entry point */
void kmain(void);

/* Hardware initialization */
void init_hardware(void);
void init_interrupts(void);
void init_memory(void);
void init_scheduler(void);

/* Memory management */
void init_memory_manager(void);

/* Kernel main loop */
void kernel_loop(void);

/* Utility functions */
void panic(const char* message);
void halt(void);

#endif /* KERNEL_H */

