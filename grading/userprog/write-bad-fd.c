#include <stdio.h>
#include <syscall.h>

int
main (void) 
{
  char buf = 123;
  printf ("(write-bad-fd) begin\n");
  write (0x20101234, &buf, 1);
  printf ("(write-bad-fd) end\n");
  return 0;
}
