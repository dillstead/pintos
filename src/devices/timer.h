#ifndef HEADER_TIMER_H
#define HEADER_TIMER_H 1

#include <stdint.h>

#define TIMER_FREQ 100

void timer_init (void);
uint64_t timer_ticks (void);
uint64_t timer_elapsed (uint64_t);

void timer_wait_until (uint64_t);

#endif /* timer.h */
