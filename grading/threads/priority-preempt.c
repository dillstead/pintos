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
#include "threads/synch.h"
#include "threads/thread.h"

static void test_preempt (void);

void
test (void) 
{
  /* Make sure our priority is the default. */
  ASSERT (thread_get_priority () == PRI_DEFAULT);

  test_preempt ();
}

static thread_func simple_thread_func;

static void
test_preempt (void) 
{
  printf ("\n"
          "Testing priority preemption.\n");
  thread_create ("high-priority", PRI_DEFAULT + 1, simple_thread_func, NULL);
  printf ("The high-priority thread should have already completed.\n"
          "Priority preemption test done.\n");
}

static void 
simple_thread_func (void *aux UNUSED) 
{
  int i;
  
  for (i = 0; i < 5; i++) 
    {
      printf ("Thread %s iteration %d\n", thread_name (), i);
      thread_yield ();
    }
  printf ("Thread %s done!\n", thread_name ());
}
