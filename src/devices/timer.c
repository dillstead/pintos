#include "devices/timer.h"
#include <debug.h>
#include "threads/interrupt.h"
#include "threads/io.h"
  
#if TIMER_FREQ < 19
#error 8254 timer requires TIMER_FREQ >= 19
#endif

/* Number of timer ticks since OS booted. */
static volatile int64_t ticks;

static intr_handler_func timer_interrupt;

/* Sets up the 8254 Programmable Interrupt Timer (PIT) to
   interrupt PIT_FREQ times per second, and registers the
   corresponding interrupt. */
void
timer_init (void) 
{
  /* 8254 input frequency divided by TIMER_FREQ, rounded to
     nearest. */
  uint16_t count = (1193180 + TIMER_FREQ / 2) / TIMER_FREQ;

  outb (0x43, 0x34);    /* CW: counter 0, LSB then MSB, mode 2, binary. */
  outb (0x40, count & 0xff);
  outb (0x40, count >> 8);

  intr_register (0x20, 0, INTR_OFF, timer_interrupt, "8254 Timer");
}

/* Returns the number of timer ticks since the OS booted. */
int64_t
timer_ticks (void) 
{
  enum intr_level old_level = intr_disable ();
  int64_t t = ticks;
  intr_set_level (old_level);
  return t;
}

/* Returns the number of timer ticks elapsed since THEN, which
   should be a value once returned by timer_ticks(). */
int64_t
timer_elapsed (int64_t then) 
{
  return timer_ticks () - then;
}

/* Suspends execution for approximately MS milliseconds. */
void
timer_msleep (int64_t ms) 
{
  int64_t ticks = (int64_t) ms * TIMER_FREQ / 1000;
  int64_t start = timer_ticks ();

  while (timer_elapsed (start) < ticks) 
    continue;
}

/* Suspends execution for approximately US microseconds.
   Note: this is ridiculously inaccurate. */
void
timer_usleep (int64_t us) 
{
  timer_msleep (us / 1000 + 1);
}

/* Suspends execution for approximately NS nanoseconds.
   Note: this is ridiculously inaccurate. */
void
timer_nsleep (int64_t ns) 
{
  timer_msleep (ns / 1000000 + 1);
}

/* Timer interrupt handler. */
static void
timer_interrupt (struct intr_frame *args UNUSED)
{
  ticks++;
  intr_yield_on_return ();
}
