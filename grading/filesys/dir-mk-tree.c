#include <stdarg.h>
#include <stdio.h>
#include <syscall.h>
#include "fslib.h"
#include "mk-tree.h"

const char test_name[] = "dir-mk-tree";

void
test_main (void) 
{
  make_tree (4, 3, 3, 4);
}

