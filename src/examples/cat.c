/* cat.c

   Prints files specified on command line to the console. */

#include <stdio.h>
#include <syscall.h>

int
main (int argc, char *argv[]) 
{
  int i;
  
  for (i = 1; i < argc; i++) 
    {
      int fd = open (argv[i]);
      if (fd < 0) 
        {
          printf ("%s: open failed\n", argv[i]);
          continue;
        }
      for (;;) 
        {
          char buffer[1024];
          int bytes_read = read (fd, buffer, sizeof buffer);
          if (bytes_read == 0)
            break;
          write (STDOUT_FILENO, buffer, bytes_read);
        }
      close (fd);
    }
  return 0;
}
