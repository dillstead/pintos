#include <syscall.h>
#include "syscall-stub.h"
#include "../syscall-nr.h"

void
halt (void)
{
  syscall (SYS_halt);
  NOT_REACHED ();
}

void
exit (int status)
{
  syscall (SYS_exit, status);
  NOT_REACHED ();
}

pid_t
exec (const char *file)
{
  return syscall (SYS_exec, file);
}

int
join (pid_t pid)
{
  return syscall (SYS_join, pid);
}

bool
create (const char *file)
{
  return syscall (SYS_create, file);
}

bool
remove (const char *file)
{
  return syscall (SYS_remove, file);
}

int
open (const char *file)
{
  return syscall (SYS_open, file);
}

int
read (int fd, void *buffer, unsigned size)
{
  return syscall (SYS_read, fd, buffer, size);
}

int
write (int fd, const void *buffer, unsigned size)
{
  return syscall (SYS_write, fd, buffer, size);
}

void
close (int fd)
{
  syscall (SYS_close, fd);
}
