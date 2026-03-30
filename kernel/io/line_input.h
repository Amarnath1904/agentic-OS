#ifndef KERNEL_IO_LINE_INPUT_H
#define KERNEL_IO_LINE_INPUT_H

#include <stddef.h>
#include <stdbool.h>

/* Block until Enter. NUL-terminates buf. Returns char count (excluding NUL). */
size_t line_input_read(char* buf, size_t cap, bool echo_chars);

#endif
