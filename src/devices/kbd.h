#ifndef DEVICES_KBD_H
#define DEVICES_KBD_H

#include <stdint.h>

void kbd_init (void);
uint8_t kbd_getc (void);

#endif /* devices/kbd.h */
