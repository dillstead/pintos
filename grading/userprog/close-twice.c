#include <stdio.h>
#include <syscall.h>

int
main (void) 
{
  int handle;
  printf ("(close-twice) begin\n");
  handle = open ("sample.txt");
  if (handle < 2)
    printf ("(close-twice) fail: open() returned %d\n", handle);
  close (handle);
  close (handle);
  printf ("(close-twice) end\n");
  return 0;
}
