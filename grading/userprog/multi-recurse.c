#include <debug.h>
#include <stdio.h>
#include <stdlib.h>
#include <syscall.h>

int
main (int argc UNUSED, char *argv[]) 
{
  int n = atoi (argv[1]);
  if (n == 0)
    n = atoi (argv[0]);
    
  printf ("(multi-recurse) begin %d\n", n);
  if (n != 0) 
    {
      char child_cmd[128];
      pid_t child_pid;
      
      snprintf (child_cmd, sizeof child_cmd, "multi-recurse %d", n - 1);
      child_pid = exec (child_cmd);
      if (child_pid != -1) 
        {
          int code = wait (child_pid);
          if (code != n - 1)
            printf ("(multi-recurse) fail: wait(exec(\"%s\")) returned %d\n",
                    child_cmd, code);
        }
      else
        printf ("(multi-recurse) fail: exec(\"%s\") returned -1\n", child_cmd);
    }
  
  printf ("(multi-recurse) end %d\n", n);
  return n;
}
