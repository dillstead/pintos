/* Problem 1-3: Priority Scheduling tests.

   Based on a test originally submitted for Stanford's CS 140 in
   winter 1999 by by Matt Franklin
   <startled@leland.stanford.edu>, Greg Hutchins
   <gmh@leland.stanford.edu>, Yu Ping Hu <yph@cs.stanford.edu>.
   Modified by arens. */

#ifdef MLFQS
#error This test not applicable with MLFQS enabled.
#endif

#include "threads/test.h"
#include <stdio.h>
#include "threads/malloc.h"
#include "threads/synch.h"
#include "threads/thread.h"

static void test_fifo (void);

void
test (void) 
{
  /* Make sure our priority is the default. */
  ASSERT (thread_get_priority () == PRI_DEFAULT);

  test_fifo ();
}

static thread_func simple_thread_func;

struct simple_thread_data 
  {
    struct lock *lock;          /* Lock on output. */
    char **out;                 /* Output pointer. */
  };

static void
test_fifo (void) 
{
  struct simple_thread_data data;
  struct lock lock;
  char *output, *cp;
  int i;
  
  printf ("\n"
          "Testing FIFO preemption.\n"
          "10 threads will iterate 5 times in the same order each time.\n"
          "If the order varies then there is a bug.\n");

  output = cp = malloc (5 * 10 * 128);
  ASSERT (output != NULL);
  lock_init (&lock, "output");

  data.lock = &lock;
  data.out = &cp;

  thread_set_priority (PRI_DEFAULT + 2);
  for (i = 0; i < 10; i++) 
    {
      char name[16];
      snprintf (name, sizeof name, "%d", i);
      thread_create (name, PRI_DEFAULT + 1, simple_thread_func, &data);
    }
  thread_set_priority (PRI_DEFAULT);

  lock_acquire (&lock);
  *cp = '\0';
  printf ("%sFIFO preemption test done.\n", output);
  lock_release (&lock);
}

static void 
simple_thread_func (void *data_) 
{
  struct simple_thread_data *data = data_;
  int i;
  
  for (i = 0; i < 5; i++) 
    {
      lock_acquire (data->lock);
      *data->out += snprintf (*data->out, 128, "Thread %s iteration %d\n",
                              thread_name (), i);
      lock_release (data->lock);
      thread_yield ();
    }

  lock_acquire (data->lock);
  *data->out += snprintf (*data->out, 128,
                          "Thread %s done!\n", thread_name ());
  lock_release (data->lock);
}
