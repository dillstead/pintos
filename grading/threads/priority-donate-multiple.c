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

static void test_donate_multiple (void);

void
test (void) 
{
  /* Make sure our priority is the default. */
  ASSERT (thread_get_priority () == PRI_DEFAULT);

  test_donate_multiple ();
}

static thread_func a_thread_func;
static thread_func b_thread_func;

static void
test_donate_multiple (void) 
{
  struct lock a, b;

  printf ("\n"
          "Testing multiple priority donation.\n"
          "If the statements printed below are all true, you pass.\n");

  lock_init (&a, "a");
  lock_init (&b, "b");

  lock_acquire (&a);
  lock_acquire (&b);

  thread_create ("a", PRI_DEFAULT + 1, a_thread_func, &a);
  printf (" 1. Main thread should have priority %d.  Actual priority: %d.\n",
          PRI_DEFAULT + 1, thread_get_priority ());

  thread_create ("b", PRI_DEFAULT + 2, b_thread_func, &b);
  printf (" 2. Main thread should have priority %d.  Actual priority: %d.\n",
          PRI_DEFAULT + 2, thread_get_priority ());

  lock_release (&b);
  printf (" 5. Thread b should have just finished.\n");
  printf (" 6. Main thread should have priority %d.  Actual priority: %d.\n",
          PRI_DEFAULT + 1, thread_get_priority ());

  lock_release (&a);
  printf (" 9. Thread a should have just finished.\n");
  printf ("10. Main thread should have priority %d.  Actual priority: %d.\n",
          PRI_DEFAULT, thread_get_priority ());
  printf ("Multiple priority priority donation test finished.\n");
}

static void
a_thread_func (void *lock_) 
{
  struct lock *lock = lock_;

  lock_acquire (lock);
  printf (" 7. Thread a acquired lock a.\n");
  lock_release (lock);
  printf (" 8. Thread a finished.\n");
}

static void
b_thread_func (void *lock_) 
{
  struct lock *lock = lock_;

  lock_acquire (lock);
  printf (" 3. Thread b acquired lock b.\n");
  lock_release (lock);
  printf (" 4. Thread b finished.\n");
}
