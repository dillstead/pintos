#include <string.h>
#include <syscall.h>
#include "fslib.h"

const char test_name[] = "dir-rm-vine";

void
test_main (void) 
{
  const char *filename = "/0/1/2/3/4/5/6/7/8/9/test";
  int fd;
  char tmp[128];
  
  tmp[1] = '\0';
  for (tmp[0] = '0'; tmp[0] <= '9'; tmp[0]++) 
    {
      check (mkdir (tmp), "mkdir \"%s\"", tmp);
      check (chdir (tmp), "chdir \"%s\"", tmp);
    }
  check (create ("test", 512), "create \"test\"");

  check (chdir ("/"), "chdir \"/\"");
  check ((fd = open (filename)) > 1, "open \"%s\"", filename);
  msg ("close \"%s\"", filename);
  close (fd);

  strlcpy (tmp, filename, sizeof tmp);
  while (strlen (tmp) > 0)
    {
      check (remove (tmp), "remove \"%s\"", tmp);
      *strrchr (tmp, '/') = 0;
    }

  check (open (filename) == -1, "open \"%s\" (must return -1)", filename);
}
