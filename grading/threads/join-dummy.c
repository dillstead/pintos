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

static void dummy_test (void);

void
test (void) 
{
  dummy_test ();
}

static thread_func simple_thread_func;

static void
dummy_test (void) 
{
  tid_t tid0;
  
  printf ("\n"
          "Testing dummy join.\n"
          "Thread 0 should finish before thread 1 starts.\n");
  tid0 = thread_create ("0", PRI_DEFAULT, simple_thread_func, "0");
  thread_yield ();
  thread_join (tid0);
  thread_join (tid0);
  simple_thread_func ("1");
  thread_join (tid0);
  printf ("Dummy join test done.\n");
}

void 
simple_thread_func (void *name_) 
{
  const char *name = name_;
  int i;
  
  for (i = 0; i < 5; i++) 
    {
      printf ("Thread %s iteration %d\n", name, i);
      thread_yield ();
    }
  printf ("Thread %s done!\n", name);
}
