#include "devices/timer.h"
#include <debug.h>
#include <inttypes.h>
#include <round.h>
#include <stdio.h>
#include "threads/interrupt.h"
#include "threads/io.h"
#include "threads/thread.h"
  
/* See [8254] for hardware details of the 8254 timer chip. */

#if TIMER_FREQ < 19
#error 8254 timer requires TIMER_FREQ >= 19
#endif
#if TIMER_FREQ > 1000
#error TIMER_FREQ <= 1000 recommended
#endif

/* Number of timer ticks that a process gets before being
   preempted. */
#define TIME_SLICE 1

/* Number of timer ticks since OS booted. */
static volatile int64_t ticks;

static intr_handler_func timer_interrupt;

/* Sets up the 8254 Programmable Interval Timer (PIT) to
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

/* Suspends execution for approximately TICKS timer ticks. */
void
timer_sleep (int64_t ticks) 
{
  int64_t start = timer_ticks ();

  ASSERT (intr_get_level () == INTR_ON);
  while (timer_elapsed (start) < ticks) 
    thread_yield ();
}

/* Returns MS milliseconds in timer ticks, rounding up. */
int64_t
timer_ms2ticks (int64_t ms) 
{
  /*       MS / 1000 s          
     ------------------------ = MS * TIMER_FREQ / 1000 ticks. 
     (1 / TIMER_FREQ) ticks/s
  */
  return DIV_ROUND_UP (ms * TIMER_FREQ, 1000);
}

/* Returns US microseconds in timer ticks, rounding up. */
int64_t
timer_us2ticks (int64_t us) 
{
  return DIV_ROUND_UP (us * TIMER_FREQ, 1000000);
}

/* Returns NS nanoseconds in timer ticks, rounding up. */
int64_t
timer_ns2ticks (int64_t ns) 
{
  return DIV_ROUND_UP (ns * TIMER_FREQ, 1000000000);
}

/* Prints timer statistics. */
void
timer_print_stats (void) 
{
  printf ("Timer: %"PRId64" ticks\n", ticks);
}

/* Timer interrupt handler. */
static void
timer_interrupt (struct intr_frame *args UNUSED)
{
  ticks++;
  thread_tick ();
  if (ticks % TIME_SLICE == 0)
    intr_yield_on_return ();
}
