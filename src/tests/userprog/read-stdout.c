#include <stdio.h>
#include <syscall.h>
#include "tests/main.h"

void
test_main (void) 
{
  char buf;
  read (STDOUT_FILENO, &buf, 1);
}
