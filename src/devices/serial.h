#ifndef DEVICES_SERIAL_H
#define DEVICES_SERIAL_H

#include <stdint.h>

void serial_init_poll (void);
void serial_init_queue (void);
void serial_putc (uint8_t);

#endif /* devices/serial.h */
