#include <stdio.h>
#include <syscall.h>

int
main (void) 
{
  int handle;
  printf ("(write-bad-ptr) begin\n");

  handle = open ("sample.txt");
  if (handle < 2)
    printf ("(write-bad-ptr) fail: open() returned %d\n", handle);

  write (handle, (char *) 0x20101234, 123);
  
  printf ("(write-bad-ptr) end\n");
  return 0;
}
