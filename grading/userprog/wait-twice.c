#include <stdio.h>
#include <syscall.h>

int
main (void) 
{
  pid_t child;
  printf ("(wait-twice) begin\n");
  child = exec ("child-simple");
  printf ("(wait-twice) wait(exec()) = %d\n", wait (child));
  wait (child);
  printf ("(wait-twice) end\n");
  return 0;
}
