#include <stdio.h>
#include <syscall.h>

int
main (void) 
{
  printf ("(close-stdin) begin\n");
  close (0);
  printf ("(close-stdin) end\n");
  return 0;
}
