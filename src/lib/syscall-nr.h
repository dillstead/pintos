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
#define SYS_read 7              /* Reads from a file. */
#define SYS_write 8             /* Writes to a file. */
#define SYS_close 9             /* Closes a file. */
#define SYS_length 10           /* Obtains a file's size. */
#define SYS_mmap 11             /* Maps a file into memory. */
#define SYS_munmap 12           /* Removes a memory mapping. */

/* Predefined file handles. */
#define STDIN_FILENO 0
#define STDOUT_FILENO 1

#endif /* lib/syscall-nr.h */
