#include <stdio.h>
#include <syscall.h>

int
main (void) 
{
  char buf;
  printf ("(read-bad-fd) begin\n");
  read (0x20101234, &buf, 1);
  printf ("(read-bad-fd) end\n");
  return 0;
}
