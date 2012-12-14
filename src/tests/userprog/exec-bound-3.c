/* Invokes an exec system call with the exec string straddling a
   page boundary such that the first byte of the string is valid
   but the remainder of the string is in invalid memory. Must
   kill process. */

#include <syscall-nr.h>
#include "tests/userprog/boundary.h"
#include "tests/lib.h"
#include "tests/main.h"

void
test_main (void) 
{
  char *p = get_bad_boundary () - 1;
  *p = 'a';
  exec(p);
  fail ("should have killed process");
}
