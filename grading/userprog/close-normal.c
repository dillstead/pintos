#include <stdio.h>
#include <syscall.h>

int
main (void) 
{
  int handle;
  printf ("(close-normal) begin\n");
  handle = open ("sample.txt");
  if (handle < 2)
    printf ("(close-normal) fail: open() returned %d\n", handle);
  close (handle);
  printf ("(close-normal) end\n");
  return 0;
}
