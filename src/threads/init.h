#ifndef THREADS_INIT_H
#define THREADS_INIT_H

#include <debug.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Page directory with kernel mappings only. */
extern uint32_t *base_page_dir;

/* -q: Power off when kernel tasks complete? */
extern bool power_off_when_done;

void power_off (void) NO_RETURN;
void reboot (void);

#endif /* threads/init.h */
