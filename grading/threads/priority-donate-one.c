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

static void test_donate_return (void);

void
test (void) 
{
  /* Make sure our priority is the default. */
  ASSERT (thread_get_priority () == PRI_DEFAULT);

  test_donate_return ();
}

static thread_func acquire_thread_func;

static void
test_donate_return (void) 
{
  struct lock lock;

  printf ("\n"
          "Testing priority donation.\n"
          "If the statements printed below are all true, you pass.\n");

  lock_init (&lock, "donor");
  lock_acquire (&lock);
  thread_create ("acquire1", PRI_DEFAULT + 1, acquire_thread_func, &lock);
  printf ("This thread should have priority %d.  Actual priority: %d.\n",
          PRI_DEFAULT + 1, thread_get_priority ());
  thread_create ("acquire2", PRI_DEFAULT + 2, acquire_thread_func, &lock);
  printf ("This thread should have priority %d.  Actual priority: %d.\n",
          PRI_DEFAULT + 2, thread_get_priority ());
  lock_release (&lock);
  printf ("acquire2 and acquire1 must already have finished, in that order.\n"
          "This should be the last line before finishing this test.\n"
          "Priority donation test done.\n");
}

static void
acquire_thread_func (void *lock_) 
{
  struct lock *lock = lock_;

  lock_acquire (lock);
  printf ("%s: got the lock\n", thread_name ());
  lock_release (lock);
  printf ("%s: done\n", thread_name ());
}
