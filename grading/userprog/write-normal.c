#include <stdio.h>
#include <string.h>
#include <syscall.h>
#include "sample.inc"

int
main (void) 
{
  int handle, byte_cnt;
  printf ("(write-normal) begin\n");

  if (!create ("test.txt", sizeof sample - 1))
    printf ("(write-normal) create() failed\n");

  handle = open ("test.txt");
  if (handle < 2)
    printf ("(write-normal) fail: open() returned %d\n", handle);

  byte_cnt = write (handle, sample, sizeof sample - 1);
  if (byte_cnt != sizeof sample - 1)
    printf ("(write-normal) fail: write() returned %d instead of %zu\n",
            byte_cnt, sizeof sample - 1);
  
  printf ("(write-normal) end\n");
  return 0;
}
