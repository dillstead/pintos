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
  test_sleep (7);
}

struct sleep_thread_data 
  {
    int64_t start;              /* Start time. */
    int duration;               /* Number of ticks to sleep. */
    int iterations;             /* Number of iterations to run. */
    struct semaphore done;      /* Completion semaphore. */
    tid_t tid;                  /* Thread ID. */
    int id;                     /* Sleeper ID. */

    struct lock *lock;          /* Lock on access to `op'. */
    int **op;                   /* Output buffer position. */
  };

static void sleeper (void *);

static void
test_sleep (int iterations) 
{
  struct sleep_thread_data threads[5];
  const int thread_cnt = sizeof threads / sizeof *threads;
  int *output, *op;
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
  op = output = malloc (sizeof *output * iterations * thread_cnt * 2);
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
      t->id = i;

      t->lock = &lock;
      t->op = &op;
    }
  
  /* Wait for all the threads to finish. */
  for (i = 0; i < thread_cnt; i++) 
    {
      sema_down (&threads[i].done);
      threads[i].iterations = 1;
    }

  /* Print output buffer. */
  product = 0;
  for (; output < op; output++) 
    {
      struct sleep_thread_data *t;
      int new_prod;
      
      ASSERT (*output >= 0 && *output < thread_cnt);
      t = threads + *output;

      new_prod = t->iterations++ * t->duration;
        
      printf ("thread %d: duration=%d, iteration=%d, product=%d\n",
              t->id, t->duration, t->iterations, new_prod);
          
      if (new_prod >= product)
        product = new_prod;
      else
        printf ("thread %d: Out of order sleep completion (%d > %d)!\n",
                t->id, product, new_prod);
    }
  
  printf ("...done\n");
}

static void
sleeper (void *t_) 
{
  struct sleep_thread_data *t = t_;
  int i;

  for (i = 1; i <= t->iterations; i++) 
    {
      timer_sleep ((t->start + i * t->duration) - timer_ticks ());

      lock_acquire (t->lock);
      *(*t->op)++ = t->id;
      lock_release (t->lock);
    }
  
  /* Signal completion. */
  sema_up (&t->done);
}
