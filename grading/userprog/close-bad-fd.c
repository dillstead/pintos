#include <stdio.h>
#include <syscall.h>

int
main (void) 
{
  printf ("(close-bad-fd) begin\n");
  close (0xc0101234);
  printf ("(close-bad-fd) end\n");
  return 0;
}
