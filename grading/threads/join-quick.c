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

static void quick_test (void);

void
test (void) 
{
  quick_test ();
}

static thread_func quick_thread_func;
static thread_func simple_thread_func;

static void
quick_test (void) 
{
  tid_t tid2;
  
  printf ("\n"
          "Testing quick join.\n"
          "Thread 2 should finish before thread 3 starts.\n");

  tid2 = thread_create ("2", PRI_DEFAULT, quick_thread_func, "2");
  thread_yield ();
  thread_join (tid2);
  simple_thread_func ("3");
  printf ("Quick join test done.\n");
}

void 
quick_thread_func (void *name_) 
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
