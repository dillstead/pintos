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

static void simple_test (void);
static void quick_test (void);
static void multiple_test (void);

void
test (void) 
{
  simple_test ();
  quick_test ();
  multiple_test ();
}

static thread_func simple_thread_func;
static thread_func quick_thread_func;

static void
simple_test (void) 
{
  tid_t tid0;
  
  printf ("\n"
          "Testing simple join.\n"
          "Thread 0 should finish before thread 1 starts.\n");
  tid0 = thread_create ("0", PRI_DEFAULT, simple_thread_func, "0");
  thread_yield ();
  thread_join (tid0);
  simple_thread_func ("1");
  printf ("Simple join test done.\n");
}

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

void 
quick_thread_func (void *name_) 
{
  const char *name = name_;
  int i;

  intr_disable ();

  for (i = 0; i < 5; i++) 
    {
      printf ("Thread %s iteration %d\n", name, i);
      thread_yield ();
    }
  printf ("Thread %s done!\n", name);
}
