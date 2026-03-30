/* ============================================
 * Blocking console line input (PS/2 via IRQ)
 * ============================================ */

#include "line_input.h"
#include "../kernel.h"
#include "../drivers/keyboard.h"
#include "../drivers/vga.h"

static bool is_printable_ascii(char c)
{
    return (unsigned char)c >= 32 && (unsigned char)c <= 126;
}

size_t line_input_read(char* buf, size_t cap, bool echo_chars)
{
    if (buf == NULL || cap < 2) {
        return 0;
    }

    size_t len = 0;
    buf[0] = '\0';

    for (;;) {
        asm volatile("sti");
        asm volatile("hlt");

        char c = keyboard_read();
        if (c == 0) {
            continue;
        }

        if (c == '\n') {
            buf[len] = '\0';
            return len;
        }

        if (c == '\b') {
            if (len > 0) {
                len--;
                buf[len] = '\0';
                vga_backspace();
            }
            continue;
        }

        if (is_printable_ascii(c) && len < cap - 1) {
            buf[len++] = c;
            buf[len] = '\0';
            if (echo_chars) {
                vga_putchar(c);
            } else {
                vga_putchar('*');
            }
        }
    }
}
