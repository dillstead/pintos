#ifndef __LIB_USER_SYSCALL_H
#define __LIB_USER_SYSCALL_H

#include <stdbool.h>
#include <debug.h>

typedef int pid_t;

void halt (void) NO_RETURN;
void exit (int status) NO_RETURN;
pid_t exec (const char *);
int join (pid_t);
bool create (const char *);
bool remove (const char *);
int open (const char *);
int read (int fd, void *, unsigned);
int write (int fd, const void *, unsigned);
void close (int fd);

#endif /* lib/user/syscall.h */
