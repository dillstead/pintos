/* Problem 1-3: Advanced Scheduler tests.

   This depends on a correctly working Alarm Clock (Problem 1-1).

   Run this test with and without the MLFQS enabled.  The
   threads' reported test should be better with MLFQS on than
   with it off.  You may have to tune the loop counts to get
   reasonable numbers.

   Based on a test originally submitted for Stanford's CS 140 in
   winter 1999 by by Matt Franklin
   <startled@leland.stanford.edu>, Greg Hutchins
   <gmh@leland.stanford.edu>, Yu Ping Hu <yph@cs.stanford.edu>.
   Modified by arens and yph. */

/* Uncomment to print progress messages. */
/*#define SHOW_PROGRESS*/

#include "threads/test.h"
#include <stdio.h>
#include <inttypes.h>
#include "threads/synch.h"
#include "threads/thread.h"
#include "devices/timer.h"

static thread_func io_thread;
static thread_func cpu_thread;
static thread_func io_cpu_thread;

void
test (void) 
{
  static thread_func *funcs[] = {io_thread, cpu_thread, io_cpu_thread};
  static const char *names[] = {"IO", "CPU", "IO & CPU"};
  struct semaphore done[3];
  int i;

  printf ("\n"
          "Testing multilevel feedback queue scheduler.\n");

  /* Start threads. */
  for (i = 0; i < 3; i++) 
    {
      sema_init (&done[i], 0, names[i]);
      thread_create (names[i], PRI_DEFAULT, funcs[i], &done[i]);
    }

  /* Wait for threads to finish. */
  for (i = 0; i < 3; i++)
    sema_down (&done[i]);
  printf ("Multilevel feedback queue scheduler test done.\n");
}

static void
cpu_thread (void *sema_) 
{
  struct semaphore *sema = sema_;
  int64_t start = timer_ticks ();
  struct lock lock;
  int i;

  lock_init (&lock, "cpu");

  for (i = 0; i < 5000; i++)
    {
      lock_acquire (&lock);
#ifdef SHOW_PROGRESS
      printf ("CPU intensive: %d\n", thread_get_priority ());
#endif
      lock_release (&lock);
    }

  printf ("CPU bound thread finished in %"PRId64" ticks.\n",
          timer_elapsed (start));
  
  sema_up (sema);
}

static void
io_thread (void *sema_) 
{
  struct semaphore *sema = sema_;
  int64_t start = timer_ticks ();
  int i;

  for (i = 0; i < 1000; i++) 
    {
      timer_sleep (10); 
#ifdef SHOW_PROGRESS
      printf ("IO intensive: %d\n", thread_get_priority ());
#endif
    }

  printf ("IO bound thread finished in %"PRId64" ticks.\n",
          timer_elapsed (start));
  
  sema_up (sema);
}

static void
io_cpu_thread (void *sema_) 
{
  struct semaphore *sema = sema_;
  struct lock lock;
  int64_t start = timer_ticks ();
  int i;

  lock_init (&lock, "io & cpu");

  for (i = 0; i < 800; i++) 
    {
      int j;
      
      timer_sleep (10);

      for (j = 0; j < 15; j++) 
        {
          lock_acquire (&lock);
#ifdef SHOW_PROGRESS
          printf ("Alternating IO/CPU: %d\n", thread_get_priority ());
#endif
          lock_release (&lock);
        }
    }

  printf ("Alternating IO/CPU thread finished in %"PRId64" ticks.\n",
          timer_elapsed (start));
  
  sema_up (sema);
}
