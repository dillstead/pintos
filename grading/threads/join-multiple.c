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

static void multiple_test (void);

void
test (void) 
{
  multiple_test ();
}

static thread_func simple_thread_func;

static void
multiple_test (void) 
{
  tid_t tid4, tid5;
  
  printf ("\n"
          "Testing multiple join.\n"
          "Threads 4 and 5 should finish before thread 6 starts.\n");

  tid4 = thread_create ("4", PRI_DEFAULT, simple_thread_func, "4");
  tid5 = thread_create ("5", PRI_DEFAULT, simple_thread_func, "5");
  thread_yield ();
  thread_join (tid4);
  thread_join (tid5);
  simple_thread_func ("6");
  printf ("Multiple join test done.\n");
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
