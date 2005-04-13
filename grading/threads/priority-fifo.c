/* Problem 1-3: Priority Scheduling tests.

   Based on a test originally submitted for Stanford's CS 140 in
   winter 1999 by by Matt Franklin
   <startled@leland.stanford.edu>, Greg Hutchins
   <gmh@leland.stanford.edu>, Yu Ping Hu <yph@cs.stanford.edu>.
   Modified by arens. */

#include "threads/test.h"
#include <stdio.h>
#include "devices/timer.h"
#include "threads/malloc.h"
#include "threads/synch.h"
#include "threads/thread.h"

static void test_fifo (void);

void
test (void) 
{
  /* This test does not work with the MLFQS. */
  ASSERT (!enable_mlfqs);

  /* Make sure our priority is the default. */
  ASSERT (thread_get_priority () == PRI_DEFAULT);

  test_fifo ();
}

static thread_func simple_thread_func;

struct simple_thread_data 
  {
    int id;                     /* Sleeper ID. */
    int iterations;             /* Iterations so far. */
    struct lock *lock;          /* Lock on output. */
    int **op;                   /* Output buffer position. */
  };

#define THREAD_CNT 10
#define ITER_CNT 5

static void
test_fifo (void) 
{
  struct simple_thread_data data[THREAD_CNT];
  struct lock lock;
  int *output, *op;
  int i;
  
  printf ("\n"
          "Testing FIFO preemption.\n"
          "%d threads will iterate %d times in the same order each time.\n"
          "If the order varies then there is a bug.\n",
          THREAD_CNT, ITER_CNT);

  output = op = malloc (sizeof *output * THREAD_CNT * ITER_CNT * 2);
  ASSERT (output != NULL);
  lock_init (&lock, "output");

  thread_set_priority (PRI_DEFAULT + 2);
  for (i = 0; i < THREAD_CNT; i++) 
    {
      char name[16];
      struct simple_thread_data *d = data + i;
      snprintf (name, sizeof name, "%d", i);
      d->id = i;
      d->iterations = 0;
      d->lock = &lock;
      d->op = &op;
      thread_create (name, PRI_DEFAULT + 1, simple_thread_func, d);
    }

  /* This should ensure that the iterations start at the
     beginning of a timer tick. */
  timer_sleep (10);
  thread_set_priority (PRI_DEFAULT);

  lock_acquire (&lock);
  for (; output < op; output++) 
    {
      struct simple_thread_data *d;

      ASSERT (*output >= 0 && *output < THREAD_CNT);
      d = data + *output;
      if (d->iterations != ITER_CNT)
        printf ("Thread %d iteration %d\n", d->id, d->iterations);
      else
        printf ("Thread %d done!\n", d->id);
      d->iterations++;
    }
  printf ("FIFO preemption test done.\n");
  lock_release (&lock);
}

static void 
simple_thread_func (void *data_) 
{
  struct simple_thread_data *data = data_;
  int i;
  
  for (i = 0; i <= ITER_CNT; i++) 
    {
      lock_acquire (data->lock);
      *(*data->op)++ = data->id;
      lock_release (data->lock);
      thread_yield ();
    }
}
