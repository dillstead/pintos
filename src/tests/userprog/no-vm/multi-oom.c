/* Recursively executes itself until the child fails to execute.
   We expect that at least 15 copies can run.
   We also require that, if a process doesn't actually get to
   start, exec() must return -1, not a valid PID. */

#include <debug.h>
#include <stdio.h>
#include <stdlib.h>
#include <syscall.h>
#include "tests/lib.h"

const char *test_name = "multi-oom";

int
main (int argc UNUSED, char *argv[]) 
{
  char child_cmd[128];
  pid_t child_pid;
  int n;

  n = atoi (argv[1]);
  if (n == 0)
    n = atoi (argv[0]);
    
  msg ("begin %d", n);

  snprintf (child_cmd, sizeof child_cmd, "multi-oom %d", n + 1);
  child_pid = exec (child_cmd);
  if (child_pid != -1) 
    {
      int code = wait (child_pid);
      if (code != n + 1)
        fail ("wait(exec(\"%s\")) returned %d", child_cmd, code);
    }
  else if (n < 15)
    fail ("exec(\"%s\") returned -1 after only %d recursions", child_cmd, n);
  
  msg ("end %d", n);
  return n;
}
