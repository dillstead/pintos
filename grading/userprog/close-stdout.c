#include <stdio.h>
#include <syscall.h>

int
main (void) 
{
  printf ("(close-stdout) begin\n");
  close (1);
  printf ("(close-stdout) end\n");
  return 0;
}
