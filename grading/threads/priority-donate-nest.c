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

static void test_donate_nest (void);

void
test (void) 
{
  /* Make sure our priority is the default. */
  ASSERT (thread_get_priority () == PRI_DEFAULT);

  test_donate_nest ();
}

static thread_func medium_thread_func;
static thread_func high_thread_func;

struct locks 
  {
    struct lock *a;
    struct lock *b;
  };

static void
test_donate_nest (void) 
{
  struct lock a, b;
  struct locks locks;

  printf ("\n"
          "Testing nested priority donation.\n"
          "If the statements printed below are all true, you pass.\n");

  lock_init (&a, "a");
  lock_init (&b, "b");

  lock_acquire (&a);

  locks.a = &a;
  locks.b = &b;
  thread_create ("medium", PRI_DEFAULT + 1, medium_thread_func, &locks);
  thread_yield ();
  printf (" 1. Low thread should have priority %d.  Actual priority: %d.\n",
          PRI_DEFAULT + 1, thread_get_priority ());

  thread_create ("high", PRI_DEFAULT + 2, high_thread_func, &b);
  thread_yield ();
  printf (" 2. Low thread should have priority %d.  Actual priority: %d.\n",
          PRI_DEFAULT + 2, thread_get_priority ());

  lock_release (&a);
  thread_yield ();
  printf (" 9. Medium thread should just have finished.\n");
  printf ("10. Low thread should have priority %d.  Actual priority: %d.\n",
          PRI_DEFAULT, thread_get_priority ());
  printf ("Nested priority priority donation test finished.\n");
}

static void
medium_thread_func (void *locks_) 
{
  struct locks *locks = locks_;

  lock_acquire (locks->b);
  lock_acquire (locks->a);

  printf (" 3. Medium thread should have priority %d.  Actual priority: %d\n",
          PRI_DEFAULT + 2, thread_get_priority ());
  printf (" 4. Medium thread got the lock.\n");

  lock_release (locks->a);
  thread_yield ();

  lock_release (locks->b);
  thread_yield ();

  printf (" 7. High thread should have just finished.\n");
  printf (" 8. Middle thread finished.\n");
}

static void
high_thread_func (void *lock_) 
{
  struct lock *lock = lock_;

  lock_acquire (lock);
  printf (" 5. High thread got the lock.\n");
  lock_release (lock);
  printf (" 6. High thread finished.\n");
}
