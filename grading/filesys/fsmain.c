#include <random.h>
#include "fslib.h"

int
main (void) 
{
  msg ("begin");
  random_init (0);
  test_main ();
  msg ("end");
  return 0;
}
