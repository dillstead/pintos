#ifndef __LIB_SYSCALL_NR_H
#define __LIB_SYSCALL_NR_H

/* System call numbers. */
#define SYS_halt 0              /* Halts the operating system. */
#define SYS_exit 1              /* Terminates this process. */
#define SYS_exec 2              /* Start another process. */
#define SYS_join 3              /* Waits for a child process to die. */
#define SYS_create 4            /* Creates a file. */
#define SYS_remove 5            /* Deletes a file. */
#define SYS_open 6              /* Opens a file. */
#define SYS_filesize 7          /* Obtains a file's size. */
#define SYS_read 8              /* Reads from a file. */
#define SYS_write 9             /* Writes to a file. */
#define SYS_close 10            /* Closes a file. */
#define SYS_mmap 12             /* Maps a file into memory. */
#define SYS_munmap 13           /* Removes a memory mapping. */
#define SYS_chdir 14            /* Changes the current directory. */
#define SYS_mkdir 15            /* Create a directory. */
#define SYS_lsdir 16            /* Lists the current directory to stdout. */

/* Predefined file handles. */
#define STDIN_FILENO 0
#define STDOUT_FILENO 1

#endif /* lib/syscall-nr.h */
