#include <stdio.h>
#include "tests/threads/tests.h"
#include "threads/init.h"
#include "threads/malloc.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include "devices/timer.h"

static int64_t start_time;

static void load_thread (void *aux);

#define THREAD_CNT 60

void
test_mlfqs_load_60 (void) 
{
  int i;
  
  ASSERT (enable_mlfqs);

  start_time = timer_ticks ();
  msg ("Starting %d niced load threads...", THREAD_CNT);
  for (i = 0; i < THREAD_CNT; i++) 
    {
      char name[16];
      snprintf(name, sizeof name, "load %d", i);
      thread_create (name, PRI_DEFAULT, load_thread, NULL);
    }
  msg ("Starting threads took %d seconds.",
       timer_elapsed (start_time) / TIMER_FREQ);
  
  for (i = 0; i < 90; i++) 
    {
      int64_t sleep_until = start_time + TIMER_FREQ * (2 * i + 10);
      int load_avg;
      timer_sleep (sleep_until - timer_ticks ());
      load_avg = thread_get_load_avg ();
      msg ("After %d seconds, load average=%d.%02d.",
           i * 2, load_avg / 100, load_avg % 100);
    }
}

static void
load_thread (void *aux UNUSED) 
{
  int64_t sleep_time = 10 * TIMER_FREQ;
  int64_t spin_time = sleep_time + 60 * TIMER_FREQ;
  int64_t exit_time = spin_time + 60 * TIMER_FREQ;

  thread_set_nice (20);
  timer_sleep (sleep_time - timer_elapsed (start_time));
  while (timer_elapsed (start_time) < spin_time)
    continue;
  timer_sleep (exit_time - timer_elapsed (start_time));
}
