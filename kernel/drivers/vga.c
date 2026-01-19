/* ============================================
 * VGA DRIVER
 * Text mode VGA driver with secure buffer management
 * ============================================ */

#include "vga.h"
#include "../kernel.h"

#define VGA_MEMORY 0xB8000
#define VGA_WIDTH  80
#define VGA_HEIGHT 25

static u8 vga_color = 0x07;  /* Light gray on black */
static u32 vga_cursor_x = 0;
static u32 vga_cursor_y = 0;

/* ============================================
 * Initialize VGA
 * ============================================ */
void vga_init(void)
{
    vga_clear();
    vga_set_cursor(0, 0);
}

/* ============================================
 * Clear Screen
 * ============================================ */
void vga_clear(void)
{
    u16* video = (u16*)VGA_MEMORY;
    u16 blank = (vga_color << 8) | ' ';
    
    for (u32 i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        video[i] = blank;
    }
    
    vga_cursor_x = 0;
    vga_cursor_y = 0;
}

/* ============================================
 * Set Color
 * ============================================ */
void vga_set_color(u8 fg, u8 bg)
{
    vga_color = (bg << 4) | (fg & 0x0F);
}

/* ============================================
 * Put Character
 * ============================================ */
void vga_putchar(char c)
{
    u16* video = (u16*)VGA_MEMORY;
    
    if (c == '\n') {
        vga_cursor_x = 0;
        vga_cursor_y++;
    } else if (c == '\r') {
        vga_cursor_x = 0;
    } else if (c == '\t') {
        vga_cursor_x = (vga_cursor_x + 4) & ~3;
    } else {
        u32 index = vga_cursor_y * VGA_WIDTH + vga_cursor_x;
        if (index < VGA_WIDTH * VGA_HEIGHT) {
            video[index] = (vga_color << 8) | c;
            vga_cursor_x++;
        }
    }
    
    /* Scroll if needed */
    if (vga_cursor_x >= VGA_WIDTH) {
        vga_cursor_x = 0;
        vga_cursor_y++;
    }
    
    if (vga_cursor_y >= VGA_HEIGHT) {
        /* Scroll screen up */
        u16* src = (u16*)(VGA_MEMORY + VGA_WIDTH * 2);
        u16* dst = (u16*)VGA_MEMORY;
        for (u32 i = 0; i < (VGA_HEIGHT - 1) * VGA_WIDTH; i++) {
            dst[i] = src[i];
        }
        
        /* Clear last line */
        u16 blank = (vga_color << 8) | ' ';
        for (u32 i = 0; i < VGA_WIDTH; i++) {
            dst[(VGA_HEIGHT - 1) * VGA_WIDTH + i] = blank;
        }
        
        vga_cursor_y = VGA_HEIGHT - 1;
    }
    
    vga_set_cursor(vga_cursor_x, vga_cursor_y);
}

/* ============================================
 * Print String
 * ============================================ */
void vga_print(const char* str)
{
    if (str == NULL) {
        return;
    }
    
    for (u32 i = 0; str[i] != '\0'; i++) {
        vga_putchar(str[i]);
    }
}

/* ============================================
 * Set Cursor Position
 * ============================================ */
void vga_set_cursor(u32 x, u32 y)
{
    if (x >= VGA_WIDTH) x = VGA_WIDTH - 1;
    if (y >= VGA_HEIGHT) y = VGA_HEIGHT - 1;
    
    vga_cursor_x = x;
    vga_cursor_y = y;
    
    /* Update hardware cursor */
    u16 pos = y * VGA_WIDTH + x;
    
    /* VGA cursor update (port 0x3D4/0x3D5) */
    outb(0x3D4, 0x0F);
    outb(0x3D5, (u8)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (u8)((pos >> 8) & 0xFF));
}

/* ============================================
 * Port I/O Functions
 * ============================================ */
static void outb(u16 port, u8 value)
{
    asm volatile("outb %0, %1" :: "a"(value), "Nd"(port));
}

