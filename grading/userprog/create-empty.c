#include <stdio.h>
#include <syscall.h>

int
main (void) 
{
  printf ("create(\"\"):\n");
  printf ("%d\n", create ("", 0));
  printf ("survived\n");
  return 0;
}
