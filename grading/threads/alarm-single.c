/* Problem 1-1: Alarm Clock tests.

   Based on a test originally submitted for Stanford's CS 140 in
   winter 1998 by Rob Baesman <rbaesman@cs.stanford.edu>, Ben
   Taskar <btaskar@cs.stanford.edu>, and Toli Kuznets
   <tolik@cs.stanford.edu>. */

#include "threads/test.h"
#include <stdio.h>
#include "threads/malloc.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include "devices/timer.h"

#ifdef MLFQS
#error This test not applicable with MLFQS enabled.
#endif

static void test_sleep (int iterations);

void
test (void) 
{
  test_sleep (1);
}

struct sleep_thread_data 
  {
    int64_t start;              /* Start time. */
    int duration;               /* Number of ticks to sleep. */
    int iterations;             /* Number of iterations to run. */
    struct semaphore done;      /* Completion semaphore. */
    tid_t tid;                  /* Thread ID. */

    struct lock *lock;          /* Lock on access to remaining members. */
    int *product;               /* Largest product so far. */
    char **out;                 /* Output pointer. */
  };

static void sleeper (void *);

static void
test_sleep (int iterations) 
{
  struct sleep_thread_data threads[5];
  const int thread_cnt = sizeof threads / sizeof *threads;
  char *output, *cp;
  struct lock lock;
  int64_t start;
  int product;
  int i;

  printf ("\n"
          "Testing %d sleeps per thread.\n"
          "If successful, product of iteration count and\n"
          "sleep duration will appear in nondescending order.\n",
          iterations);

  /* Start all the threads. */
  product = 0;
  lock_init (&lock, "product");
  cp = output = malloc (128 * iterations * thread_cnt);
  ASSERT (output != NULL);
  start = timer_ticks ();
  for (i = 0; i < thread_cnt; i++)
    {
      struct sleep_thread_data *t;
      char name[16];
      
      snprintf (name, sizeof name, "thread %d", i);
      t = threads + i;
      t->start = start;
      t->duration = (i + 1) * 10;
      t->iterations = iterations;
      sema_init (&t->done, 0, name);
      t->tid = thread_create (name, PRI_DEFAULT, sleeper, t);

      t->lock = &lock;
      t->product = &product;
      t->out = &cp;
    }
  
  /* Wait for all the threads to finish. */
  for (i = 0; i < thread_cnt; i++) 
    sema_down (&threads[i].done);
  
  printf ("%s...done\n", output);
}

static void
sleeper (void *t_) 
{
  struct sleep_thread_data *t = t_;
  int i;

  for (i = 1; i <= t->iterations; i++) 
    {
      int old_product;
      int new_product = i * t->duration;

      timer_sleep ((t->start + new_product) - timer_ticks ());

      lock_acquire (t->lock);
      old_product = *t->product;
      *t->product = new_product;
      *t->out += snprintf (*t->out, 128,
                           "%s: duration=%d, iteration=%d, product=%d\n",
                           thread_name (), t->duration, i, new_product);
      if (old_product > new_product)
        *t->out += snprintf (*t->out, 128,
                             "%s: Out of order sleep completion (%d > %d)!\n",
                             thread_name (), old_product, new_product);
      lock_release (t->lock);
    }
  
  /* Signal completion. */
  sema_up (&t->done);
}
