#ifndef DEVICES_KBD_H
#define DEVICES_KBD_H

#include <stdint.h>

void kbd_init (void);
uint8_t kbd_getc (void);
void kbd_print_stats (void);

#endif /* devices/kbd.h */
