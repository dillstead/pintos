/* mkdir.c

   Creates a directory. */

#include <stdio.h>
#include <syscall.h>

int
main (int argc, char *argv[]) 
{
  if (argc != 2)
    PANIC ("usage: %s DIRECTORY\n", argv[0]);

  if (!mkdir (argv[1]))
    PANIC ("%s: mkdir failed\n", argv[1]);

  return 0;
}
