#include <stdio.h>
#include <string.h>
#include "arc4.h"
#include "cksum.h"

int
main (void) 
{
  char stk_obj[65536];
  struct arc4 arc4;

  printf ("(pt-big-stk-obj) begin\n");
  arc4_init (&arc4, "foobar", 6);
  memset (stk_obj, 0, sizeof stk_obj);
  arc4_crypt (&arc4, stk_obj, sizeof stk_obj);
  printf ("(pt-big-stk-obj) cksum: %lu\n", cksum (stk_obj, sizeof stk_obj));
  printf ("(pt-big-stk-obj) end\n");
  return 0;
}
