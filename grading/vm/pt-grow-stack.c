#include <stdio.h>
#include <string.h>
#include "arc4.h"
#include "cksum.h"

int
main (void) 
{
  char stack_obj[4096];
  struct arc4 arc4;

  printf ("(pt-grow-stack) begin\n");
  arc4_init (&arc4, "foobar", 6);
  memset (stack_obj, 0, sizeof stack_obj);
  arc4_crypt (&arc4, stack_obj, sizeof stack_obj);
  printf ("(pt-grow-stack) cksum: %lu\n", cksum (stack_obj, sizeof stack_obj));
  printf ("(pt-grow-stack) end\n");
  return 0;
}
