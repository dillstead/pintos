#include <debug.h>
#include <stdio.h>
#include <stdlib.h>
#include <syscall.h>

int
main (int argc UNUSED, char *argv[]) 
{
  char child_cmd[128];
  pid_t child_pid;
  int n;

  n = atoi (argv[1]);
  if (n == 0)
    n = atoi (argv[0]);
    
  printf ("(multi-oom) begin %d\n", n);

  snprintf (child_cmd, sizeof child_cmd, "multi-oom %d", n + 1);
  child_pid = exec (child_cmd);
  if (child_pid != -1) 
    {
      int code = wait (child_pid);
      if (code != n + 1)
        printf ("(multi-oom) fail: wait(exec(\"%s\")) returned %d\n",
                child_cmd, code);
    }
  else if (n < 15)
    printf ("(multi-oom) fail: exec(\"%s\") returned -1 "
            "after only %d recursions\n",
            child_cmd, n);
  
  printf ("(multi-oom) end %d\n", n);
  return n;
}
