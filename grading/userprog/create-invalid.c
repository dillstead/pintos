#include <stdio.h>
#include <syscall.h>

int
main (void) 
{
  printf ("create(0xc0101234):\n");
  printf ("%d\n", create ((char *) 0xc0101234, 0));
  printf ("survived\n");
  return 0;
}
