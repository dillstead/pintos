#include <stdio.h>
#include <syscall.h>

int
main (void) 
{
  printf ("(halt) begin\n");
  halt ();
  printf ("(halt) fail\n");
  return 0;
}
