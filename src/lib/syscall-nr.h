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
#define SYS_seek 10             /* Change position in a file. */
#define SYS_tell 11             /* Report current position in a file. */
#define SYS_close 12            /* Closes a file. */
#define SYS_mmap 13             /* Maps a file into memory. */
#define SYS_munmap 14           /* Removes a memory mapping. */
#define SYS_chdir 15            /* Changes the current directory. */
#define SYS_mkdir 16            /* Create a directory. */
#define SYS_lsdir 17            /* Lists the current directory to stdout. */

#endif /* lib/syscall-nr.h */
