#ifndef DEVICES_TIMER_H
#define DEVICES_TIMER_H

#include <round.h>
#include <stdint.h>

/* Number of timer interrupts per second. */
#define TIMER_FREQ 100

void timer_init (void);
int64_t timer_ticks (void);
int64_t timer_elapsed (int64_t);

void timer_sleep (int64_t ticks);

int64_t timer_ms2ticks (int64_t ms);
int64_t timer_us2ticks (int64_t us);
int64_t timer_ns2ticks (int64_t ns);

void timer_print_stats (void);

#endif /* devices/timer.h */
