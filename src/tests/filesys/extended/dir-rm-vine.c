/* Creates a "vine" of directories /0/1/2/3/4/5/6/7/8/9
   and changes directory into each of them,
   then remove them. */

#include <string.h>
#include <syscall.h>
#include "tests/lib.h"
#include "tests/main.h"

void
test_main (void) 
{
  const char *file_name = "/0/1/2/3/4/5/6/7/8/9/test";
  int fd;
  char tmp[128];
  
  tmp[1] = '\0';
  for (tmp[0] = '0'; tmp[0] <= '9'; tmp[0]++) 
    {
      CHECK (mkdir (tmp), "mkdir \"%s\"", tmp);
      CHECK (chdir (tmp), "chdir \"%s\"", tmp);
    }
  CHECK (create ("test", 512), "create \"test\"");

  CHECK (chdir ("/"), "chdir \"/\"");
  CHECK ((fd = open (file_name)) > 1, "open \"%s\"", file_name);
  msg ("close \"%s\"", file_name);
  close (fd);

  strlcpy (tmp, file_name, sizeof tmp);
  while (strlen (tmp) > 0)
    {
      CHECK (remove (tmp), "remove \"%s\"", tmp);
      *strrchr (tmp, '/') = 0;
    }

  CHECK (open (file_name) == -1, "open \"%s\" (must return -1)", file_name);
}
