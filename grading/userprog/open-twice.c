#include <stdio.h>
#include <syscall.h>

int
main (void) 
{
  int h1, h2;
  printf ("(open-twice) begin\n");

  h1 = open ("sample.txt");
  if (h1 < 2)
    printf ("(open-twice) fail: open() returned %d first time\n", h1);

  h2 = open ("sample.txt");
  if (h2 < 2)
    printf ("(open-twice) fail: open() returned %d second time\n", h2);
  if (h1 == h2)
    printf ("(open-twice) fail: open() returned %d both times\n", h1);

  printf ("(open-twice) end\n");
  return 0;
}
