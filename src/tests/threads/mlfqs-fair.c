#include <stdio.h>
#include <inttypes.h>
#include "tests/threads/tests.h"
#include "threads/init.h"
#include "threads/malloc.h"
#include "threads/palloc.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include "devices/timer.h"

static void test_mlfqs_fair (int thread_cnt, int nice_min, int nice_step);

void
test_mlfqs_fair_2 (void) 
{
  test_mlfqs_fair (2, 0, 0);
}

void
test_mlfqs_fair_20 (void) 
{
  test_mlfqs_fair (20, 0, 0);
}

void
test_mlfqs_nice_2 (void) 
{
  test_mlfqs_fair (2, 0, 5);
}

void
test_mlfqs_nice_10 (void) 
{
  test_mlfqs_fair (10, 0, 1);
}

#define MAX_THREAD_CNT 20

struct thread_info 
  {
    int64_t start_time;
    int tick_count;
    int nice;
  };

static void load_thread (void *aux);

static void
test_mlfqs_fair (int thread_cnt, int nice_min, int nice_step)
{
  struct thread_info info[MAX_THREAD_CNT];
  int64_t start_time;
  int nice;
  int i;

  ASSERT (enable_mlfqs);
  ASSERT (thread_cnt <= MAX_THREAD_CNT);
  ASSERT (nice_min >= -10);
  ASSERT (nice_step >= 0);
  ASSERT (nice_min + nice_step * (thread_cnt - 1) <= 20);

  thread_set_nice (-20);

  start_time = timer_ticks ();
  msg ("Starting %d threads...", thread_cnt);
  nice = nice_min;
  for (i = 0; i < thread_cnt; i++) 
    {
      struct thread_info *ti = &info[i];
      char name[16];

      ti->start_time = start_time;
      ti->tick_count = 0;
      ti->nice = nice;

      snprintf(name, sizeof name, "load %d", i);
      thread_create (name, PRI_DEFAULT, load_thread, ti);

      nice += nice_step;
    }
  msg ("Starting threads took %"PRId64" ticks.", timer_elapsed (start_time));

  msg ("Sleeping 40 seconds to let threads run, please wait...");
  timer_sleep (40 * TIMER_FREQ);
  
  for (i = 0; i < thread_cnt; i++)
    msg ("Thread %d received %d ticks.", i, info[i].tick_count);
}

static void
load_thread (void *ti_) 
{
  struct thread_info *ti = ti_;
  int64_t sleep_time = 5 * TIMER_FREQ;
  int64_t spin_time = sleep_time + 30 * TIMER_FREQ;
  int64_t last_time = 0;

  thread_set_nice (ti->nice);
  timer_sleep (sleep_time - timer_elapsed (ti->start_time));
  while (timer_elapsed (ti->start_time) < spin_time) 
    {
      int64_t cur_time = timer_ticks ();
      if (cur_time != last_time)
        ti->tick_count++;
      last_time = cur_time;
    }
}
