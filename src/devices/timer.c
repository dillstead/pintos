#include "timer.h"
#include "debug.h"
#include "interrupt.h"
#include "io.h"
  
static volatile uint64_t ticks;

static void
irq20_timer (struct intr_args *args UNUSED)
{
  ticks++;
  intr_yield_on_return ();
}

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

  intr_register (0x20, 0, IF_OFF, irq20_timer);
}

uint64_t
timer_ticks (void) 
{
  enum if_level old_level = intr_disable ();
  uint64_t t = ticks;
  intr_set_level (old_level);
  return t;
}

uint64_t
timer_elapsed (uint64_t then) 
{
  uint64_t now = timer_ticks ();
  return now > then ? now - then : ((uint64_t) -1 - then) + now + 1;
}

/* Suspends execution for at least DURATION ticks. */
void
timer_wait_until (uint64_t duration) 
{
  /* FIXME */
}
