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
create (const char *file, unsigned initial_size)
{
  return syscall (SYS_create, file, initial_size);
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
filesize (int fd) 
{
  return syscall (SYS_filesize, fd);
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
seek (int fd, unsigned position) 
{
  syscall (SYS_seek, fd, position);
}

unsigned
tell (int fd) 
{
  return syscall (SYS_tell, fd);
}

void
close (int fd)
{
  syscall (SYS_close, fd);
}

bool
mmap (int fd, void *addr, unsigned length)
{
  return syscall (SYS_mmap, fd, addr, length);
}

bool
munmap (void *addr, unsigned length)
{
  return syscall (SYS_munmap, addr, length);
}

bool
chdir (const char *dir)
{
  return syscall (SYS_chdir, dir);
}

bool
mkdir (const char *dir)
{
  return syscall (SYS_mkdir, dir);
}

void
lsdir (void)
{
  syscall (SYS_lsdir);
}

