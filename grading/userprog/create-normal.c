#include <stdio.h>
#include <syscall.h>

int
main (void) 
{
  printf ("create(\"quux.dat\"): ");
  printf ("%d\n", create ("quux.dat", 0));
  printf ("survived\n");
  return 0;
}
