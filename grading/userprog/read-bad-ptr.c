#include <stdio.h>
#include <syscall.h>

int
main (void) 
{
  int handle;
  printf ("(read-bad-ptr) begin\n");

  handle = open ("sample.txt");
  if (handle < 2)
    printf ("(read-bad-ptr) fail: open() returned %d\n", handle);

  read (handle, (char *) 0xc0101234, 123);
  
  printf ("(read-bad-ptr) end\n");
  return 0;
}
