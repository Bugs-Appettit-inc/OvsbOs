#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>

void keyboard_init(void);
void keyboard_handler(void);
char keyboard_read(void);
int keyboard_avail(void); /* ♥ nombre consistente com a implementação, seu baka! */

#endif
