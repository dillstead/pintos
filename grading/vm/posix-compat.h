#ifndef POSIX_COMPAT_H
#define POSIX_COMPAT_H

#include <stdbool.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#define NO_RETURN __attribute__ ((noreturn))

#undef halt
#define halt pintos_halt
void pintos_halt (void) NO_RETURN;

#undef exit
#define exit pintos_exit
void pintos_exit (int status) NO_RETURN;

#undef exec
#define exec pintos_exec
pid_t pintos_exec (const char *file);

#undef join
#define join pintos_join
int pintos_join (pid_t);

#undef create
#define create pintos_create
bool pintos_create (const char *file, unsigned initial_size);

#undef remove
#define remove pintos_remove
bool pintos_remove (const char *file);

#undef open
#define open pintos_open
int pintos_open (const char *file);

#undef filesize
#define filesize pintos_filesize
int pintos_filesize (int fd);

#undef read
#define read pintos_read
int pintos_read (int fd, void *buffer, unsigned length);

#undef write
#define write pintos_write
int pintos_write (int fd, const void *buffer, unsigned length);

#undef seek
#define seek pintos_seek
void pintos_seek (int fd, unsigned position);

#undef tell
#define tell pintos_tell
unsigned pintos_tell (int fd);

#undef close
#define close pintos_close
void pintos_close (int fd);

#undef mmap
#define mmap pintos_mmap
bool pintos_mmap (int fd, void *addr, unsigned length);

#undef munmap
#define munmap pintos_munmap
bool pintos_munmap (void *addr, unsigned length);

#undef chdir
#define chdir pintos_chdir
bool pintos_chdir (const char *dir);

#undef mkdir
#define mkdir pintos_mkdir
bool pintos_mkdir (const char *dir);

#undef lsdir
#define lsdir pintos_lsdir
void pintos_lsdir (void);

#endif /* posix-compat.h */
