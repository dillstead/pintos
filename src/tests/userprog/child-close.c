#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <syscall.h>
#include "tests/lib.h"

const char *test_name = "child-close";

int
main (int argc UNUSED, char *argv[]) 
{
  msg ("begin");
  if (!isdigit (*argv[1]))
    fail ("bad command-line arguments");
  close (atoi (argv[1]));
  msg ("end");

  return 0;
}
