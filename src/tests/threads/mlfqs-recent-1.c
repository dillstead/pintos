#include <stdio.h>
#include "tests/threads/tests.h"
#include "threads/init.h"
#include "threads/malloc.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include "devices/timer.h"

/* Sensitive to assumption that recent_cpu updates happen exactly
   when timer_ticks()%TIMER_FREQ == 0. */

void
test_mlfqs_recent_1 (void) 
{
  int64_t start_time;
  int last_elapsed = 0;
  
  ASSERT (enable_mlfqs);

  msg ("Sleeping 10 seconds to allow recent_cpu to decay, please wait...");
  start_time = timer_ticks ();
  timer_sleep (DIV_ROUND_UP (start_time, TIMER_FREQ) - start_time
               + 10 * TIMER_FREQ);

  start_time = timer_ticks ();
  for (;;) 
    {
      int elapsed = timer_elapsed (start_time);
      if (elapsed % (TIMER_FREQ * 2) == 0 && elapsed > last_elapsed) 
        {
          int recent_cpu = thread_get_recent_cpu ();
          int load_avg = thread_get_load_avg ();
          int elapsed_seconds = elapsed / TIMER_FREQ;
          msg ("After %d seconds, recent_cpu is %d.%02d, load_avg is %d.%02d.",
               elapsed_seconds,
               recent_cpu / 100, recent_cpu % 100,
               load_avg / 100, load_avg % 100);
          if (elapsed_seconds >= 180)
            break;
        } 
      last_elapsed = elapsed;
    }
}
