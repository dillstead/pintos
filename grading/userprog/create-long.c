#include <stdio.h>
#include <string.h>
#include <syscall.h>

int
main (void) 
{
  static char name[256];
  memset (name, 'x', sizeof name);
  name[sizeof name - 1] = '\0';
  
  printf ("create(\"%s\"):\n", name);
  printf ("%d\n", create (name, 0));
  printf ("survived\n");

  return 0;
}
