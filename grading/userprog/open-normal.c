#include <stdio.h>
#include <syscall.h>

int
main (void) 
{
  int handle;
  printf ("(open-normal) begin\n");
  handle = open ("sample.txt");
  if (handle < 2)
    printf ("(open-normal) fail: open() returned %d\n", handle);
  printf ("(open-normal) end\n");
  return 0;
}
