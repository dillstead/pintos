#ifndef DEVICES_SERIAL_H
#define DEVICES_SERIAL_H

#include <stdint.h>

void serial_init (int phase);
void serial_putc (uint8_t);
uint8_t serial_getc (void);

#endif /* devices/serial.h */
