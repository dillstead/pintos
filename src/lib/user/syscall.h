#ifndef __LIB_USER_SYSCALL_H
#define __LIB_USER_SYSCALL_H

#include <stdbool.h>
#include <debug.h>

typedef int pid_t;

void halt (void) NO_RETURN;
void exit (int status) NO_RETURN;
pid_t exec (const char *file);
int join (pid_t);
bool create (const char *file);
bool remove (const char *file);
int open (const char *file);
int filesize (int fd);
int read (int fd, void *buffer, unsigned length);
int write (int fd, const void *buffer, unsigned length);
void seek (int fd, unsigned position);
unsigned tell (int fd);
void close (int fd);
bool mmap (int fd, void *addr, unsigned length);
bool munmap (void *addr, unsigned length);
bool chdir (const char *dir);
bool mkdir (const char *dir);
void lsdir (void);

#endif /* lib/user/syscall.h */
