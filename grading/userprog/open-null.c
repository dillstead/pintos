#include <stdio.h>
#include <syscall.h>

int
main (void) 
{
  printf ("(open-null) begin\n");
  open (NULL);
  printf ("(open-null) end\n");
  return 0;
}
