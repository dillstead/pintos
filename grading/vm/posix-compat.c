#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include "posix-compat.h"

#undef halt
#undef exit
#undef exec
#undef join
#undef create
#undef remove
#undef open
#undef filesize
#undef read
#undef write
#undef seek
#undef tell
#undef close
#undef mmap
#undef munmap
#undef chdir
#undef mkdir
#undef lsdir

void
pintos_halt (void) 
{
  exit (-1);
}

void
pintos_exit (int status) 
{
  if (status < -127 || status > 255)
    printf ("warning: non-POSIX exit code %d\n", status);
  exit (status);
}

pid_t
pintos_exec (const char *cmd_line) 
{
  pid_t child = fork ();
  if (child == -1)
    return child;
  else if (child == 0) 
    {
      char copy[1024];
      char *args[128];
      char **argp;

      strcpy (copy, cmd_line);

      argp = args;
      *argp = strtok (copy, " ");
      while (*argp++ != NULL) 
        *argp = strtok (NULL, " ");

      execv (args[0], args);
      abort ();
    }
  else
    return child;
}

int
pintos_join (pid_t child) 
{
  int status = 0;
  waitpid (child, &status, 0);
  return WIFEXITED (status) ? WEXITSTATUS (status) : -1;
}

bool
pintos_create (const char *file, unsigned initial_size) 
{
  int fd = open (file, O_RDWR | O_CREAT | O_EXCL, 0666);
  if (fd < 0)
    return false;
  ftruncate (fd, initial_size);
  close (fd);
  return true;
}

bool
pintos_remove (const char *file) 
{
  return unlink (file) == 0;
}

int
pintos_open (const char *file) 
{
  return open (file, O_RDWR);
}

int
pintos_filesize (int fd) 
{
  off_t old = lseek (fd, 0, SEEK_CUR);
  off_t size = lseek (fd, 0, SEEK_END);
  lseek (fd, old, SEEK_SET);
  return size;
}

int
pintos_read (int fd, void *buffer, unsigned length) 
{
  return read (fd, buffer, length);
}

int
pintos_write (int fd, const void *buffer, unsigned length) 
{
  return write (fd, buffer, length);
}

void
pintos_seek (int fd, unsigned position) 
{
  lseek (fd, position, SEEK_SET);
}


unsigned
pintos_tell (int fd) 
{
  return lseek (fd, 0, SEEK_CUR);
}

void
pintos_close (int fd) 
{
  close (fd);
}

bool
pintos_mmap (int fd, void *addr, unsigned length) 
{
  return mmap (addr, length, PROT_READ | PROT_WRITE,
               MAP_SHARED | MAP_FIXED, fd, 0) == addr;
}

bool
pintos_munmap (void *addr, unsigned length) 
{
  return munmap (addr, length) == 0;
}

bool
pintos_chdir (const char *dir) 
{
  return chdir (dir) == 0;
}

bool
pintos_mkdir (const char *dir) 
{
  return mkdir (dir, 0777) == 0;
}

void
pintos_lsdir (void) 
{
  DIR *dir = opendir (".");
  if (dir != NULL) 
    {
      struct dirent *de;
      while ((de = readdir (dir)) != NULL) 
        {
          if (!strcmp (de->d_name, ".") || !strcmp (de->d_name, ".."))
            continue;
          printf ("%s\n", de->d_name);
        }
    }
  closedir (dir);
}
