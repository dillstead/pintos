#include <stdio.h>
#include <syscall.h>

int
main (void) 
{
  printf ("(create-exists) begin\n");
  printf ("(create-exists) create(\"quux.dat\"): %d\n",
          create ("quux.dat", 0));
  printf ("(create-exists) create(\"warble.dat\"): %d\n",
          create ("warble.dat", 0));
  printf ("(create-exists) create(\"quux.dat\"): %d\n",
          create ("quux.dat", 0));
  printf ("(create-exists) end\n");
  return 0;
}
