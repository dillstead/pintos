#include "threads/test.h"
#include <stdio.h>
#include "threads/synch.h"
#include "threads/thread.h"
#include "devices/timer.h"

static void test_sleep_once (void);

void
test (void) 
{
  test_sleep_once ();
//  test_sleep_multiple ();
}

struct sleep_once_data 
  {
    int duration;               /* Number of ticks to sleep. */
    volatile int *counter;      /* Counter to check. */
    struct semaphore done;      /* Completion semaphore. */
    struct thread *thread;      /* Thread. */
  };

static void sleep_once (void *);

static void
test_sleep_once (void) 
{
  struct sleep_once_data t[5];
  const int t_cnt = sizeof t / sizeof *t;
  volatile int counter;
  int i;

  printf ("\nTesting one sleep per thread...\n");

  /* Start all the threads. */
  counter = 0;
  for (i = 0; i < t_cnt; i++)
    {
      char name[16];
      snprintf (name, sizeof name, "once %d", i);

      t[i].duration = i * 10;
      t[i].counter = &counter;
      sema_init (&t[i].done, 0, name);
      t[i].thread = thread_create (name, sleep_once, &t[i]);
    }
  
  /* Wait for all the threads to finish. */
  for (i = 0; i < t_cnt; i++) 
    {
#ifdef THREAD_JOIN_IMPLEMENTED
      thread_join (t[i].thread);
#else
      sema_down (&t[i].done);
#endif
    }

  printf ("...done\n");
}

static void
sleep_once (void *t_) 
{
  struct sleep_once_data *t = t_;

  /* Sleep. */
  timer_sleep (t->duration);

  /* Check ordering. */
  if (*t->counter > t->duration)
    printf ("%s: Out of order sleep completion (%d > %d)!\n",
            thread_name (thread_current ()),
            *t->counter, t->duration);
  else 
    {
      *t->counter = t->duration;
      printf ("%s: Sleep %d complete\n",
              thread_name (thread_current ()), t->duration);
    }

  /* Signal completion. */
  sema_up (&t->done);
}

