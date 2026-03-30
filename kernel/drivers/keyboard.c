/* ============================================
 * KEYBOARD DRIVER
 * PS/2 keyboard interrupt handler
 * ============================================ */

#include "keyboard.h"
#include "../kernel.h"
#include "../io.h"

/* Forward declarations */
static char scancode_to_char(u8 scancode);

#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64

static char key_buffer[256];
static u32 key_buffer_head = 0;
static u32 key_buffer_tail = 0;

/* ============================================
 * Initialize Keyboard
 * ============================================ */
void keyboard_init(void)
{
    key_buffer_head = 0;
    key_buffer_tail = 0;
    
    /* Enable keyboard interrupts */
    /* IRQ1 will be enabled in interrupt setup */
}

/* ============================================
 * Keyboard Interrupt Handler
 * ============================================ */
void keyboard_handler(void)
{
    u8 scancode = inb(KEYBOARD_DATA_PORT);
    
    /* Handle key press (ignore releases for now) */
    if (scancode < 0x80) {
        char key = scancode_to_char(scancode);
        if (key != 0) {
            /* Add to buffer */
            u32 next = (key_buffer_tail + 1) % 256;
            if (next != key_buffer_head) {
                key_buffer[key_buffer_tail] = key;
                key_buffer_tail = next;
            }
        }
    }
}

/* ============================================
 * Read Key
 * ============================================ */
char keyboard_read(void)
{
    if (key_buffer_head == key_buffer_tail) {
        return 0;  /* No key available */
    }
    
    char key = key_buffer[key_buffer_head];
    key_buffer_head = (key_buffer_head + 1) % 256;
    
    return key;
}

/* ============================================
 * Scancode to Character Conversion
 * ============================================ */
static char scancode_to_char(u8 scancode)
{
    switch (scancode) {
        case 0x1E: return 'a';
        case 0x30: return 'b';
        case 0x2E: return 'c';
        case 0x20: return 'd';
        case 0x12: return 'e';
        case 0x21: return 'f';
        case 0x22: return 'g';
        case 0x23: return 'h';
        case 0x17: return 'i';
        case 0x24: return 'j';
        case 0x25: return 'k';
        case 0x26: return 'l';
        case 0x32: return 'm';
        case 0x31: return 'n';
        case 0x18: return 'o';
        case 0x19: return 'p';
        case 0x10: return 'q';
        case 0x13: return 'r';
        case 0x1F: return 's';
        case 0x14: return 't';
        case 0x16: return 'u';
        case 0x2F: return 'v';
        case 0x11: return 'w';
        case 0x2D: return 'x';
        case 0x15: return 'y';
        case 0x2C: return 'z';
        case 0x0B: return '0';
        case 0x02: return '1';
        case 0x03: return '2';
        case 0x04: return '3';
        case 0x05: return '4';
        case 0x06: return '5';
        case 0x07: return '6';
        case 0x08: return '7';
        case 0x09: return '8';
        case 0x0A: return '9';
        case 0x1C: return '\n';  /* Enter */
        case 0x0E: return '\b';  /* Backspace */
        case 0x39: return ' ';   /* Space */
        default: return 0;
    }
}
