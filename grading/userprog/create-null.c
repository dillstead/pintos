#include <stdio.h>
#include <syscall.h>

int
main (void) 
{
  printf ("create(null): ");
  printf ("%d\n", create (NULL, 0));
  printf ("survived\n");
  return 0;
}
