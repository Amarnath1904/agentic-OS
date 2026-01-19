#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "../kernel.h"

/* ============================================
 * KEYBOARD DRIVER HEADER
 * ============================================ */

void keyboard_init(void);
void keyboard_handler(void);
char keyboard_read(void);

#endif /* KEYBOARD_H */

