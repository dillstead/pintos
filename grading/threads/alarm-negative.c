/* Problem 1-1: Alarm Clock tests.

   Tests timer_sleep(-100).  Only requirement is that it not crash. */

#include "threads/test.h"
#include <stdio.h>
#include "threads/malloc.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include "devices/timer.h"

void
test (void) 
{
  printf ("\n"
          "Testing timer_sleep(-100).\n");
  timer_sleep (-100);
  printf ("Success.\n");
}
