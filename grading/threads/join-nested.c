/* Problem 1-2: Join tests.

   Based on a test originally submitted for Stanford's CS 140 in
   winter 1998 by Rob Baesman <rbaesman@cs.stanford.edu>, Ben
   Taskar <btaskar@cs.stanford.edu>, and Toli Kuznets
   <tolik@cs.stanford.edu>.  Later modified by shiangc, yph, and
   arens. */
#include "threads/test.h"
#include <stdio.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

static void nested_test (void);

void
test (void) 
{
  nested_test ();
}

static thread_func nested_thread_func;

static void
nested_test (void) 
{
  tid_t tid0;
  int zero = 0;
  
  printf ("\n"
          "Testing nested join.\n"
          "Threads 0 to 7 should start in numerical order\n"
          "and finish in reverse order.\n");
  tid0 = thread_create ("0", PRI_DEFAULT, nested_thread_func, &zero);
  thread_join (tid0);
  printf ("Nested join test done.\n");
}

void 
nested_thread_func (void *valuep_) 
{
  int *valuep = valuep_;
  int value = *valuep;

  printf ("Thread %d starting.\n", value);
  if (value < 7)
    {
      int next = value + 1;
      tid_t tid_next;
      char name_next[8];
      snprintf (name_next, sizeof name_next, "%d", next);

      tid_next = thread_create (name_next, PRI_DEFAULT,
                                nested_thread_func, &next);

      thread_join (tid_next);
    }
  printf ("Thread %d done.\n", value);
}
