#ifndef DEVICES_TIMER_H
#define DEVICES_TIMER_H

#include <stdint.h>

#define TIMER_FREQ 100

void timer_init (void);
int64_t timer_ticks (void);
int64_t timer_elapsed (int64_t);

void timer_msleep (int64_t ms);
void timer_usleep (int64_t us);
void timer_nsleep (int64_t ns);

#endif /* devices/timer.h */
