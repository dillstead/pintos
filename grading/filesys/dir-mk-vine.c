#include <syscall.h>
#include "fslib.h"

const char test_name[] = "dir-mk-vine";

void
test_main (void) 
{
  const char *filename = "/0/1/2/3/4/5/6/7/8/9/test";
  char dir[2];
  
  dir[1] = '\0';
  for (dir[0] = '0'; dir[0] <= '9'; dir[0]++) 
    {
      check (mkdir (dir), "mkdir \"%s\"", dir);
      check (chdir (dir), "chdir \"%s\"", dir);
    }
  check (create ("test", 512), "create \"test\"");
  check (chdir ("/"), "chdir \"/\"");
  check (open (filename) > 1, "open \"%s\"", filename);
}

