#include <stdio.h>
#include <string.h>
#include <syscall.h>
#include "sample.inc"

char actual[sizeof sample];

int
main (void) 
{
  int handle, byte_cnt;
  printf ("(read-normal) begin\n");

  handle = open ("sample.txt");
  if (handle < 2)
    printf ("(read-normal) fail: open() returned %d\n", handle);

  byte_cnt = read (handle, actual, sizeof actual - 1);
  if (byte_cnt != sizeof actual - 1)
    printf ("(read-normal) fail: read() returned %d instead of %zu\n",
            byte_cnt, sizeof actual - 1);
  else if (strcmp (sample, actual))
    printf ("(read-normal) fail: expected text differs from actual:\n%s",
            actual);
  
  printf ("(read-normal) end\n");
  return 0;
}
