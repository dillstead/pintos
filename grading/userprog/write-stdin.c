#include <stdio.h>
#include <syscall.h>

int
main (void) 
{
  char buf = 123;
  printf ("(write-stdin) begin\n");
  write (0, &buf, 1);
  printf ("(write-stdin) end\n");
  return 0;
}
