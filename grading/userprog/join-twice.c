#include <stdio.h>
#include <syscall.h>

int
main (void) 
{
  pid_t child;
  printf ("(join-twice) begin\n");
  child = exec ("child-simple");
  printf ("(join-twice) join(exec()) = %d\n", join (child));
  join (child);
  printf ("(join-twice) end\n");
  return 0;
}
