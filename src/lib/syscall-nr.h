#ifndef __LIB_SYSCALL_NR_H
#define __LIB_SYSCALL_NR_H

/* System call numbers. */
enum 
  {
    /* Projects 2 and later. */
    SYS_halt,                   /* Halts the operating system. */
    SYS_exit,                   /* Terminates this process. */
    SYS_exec,                   /* Start another process. */
    SYS_wait,                   /* Waits for a child process to die. */
    SYS_create,                 /* Creates a file. */
    SYS_remove,                 /* Deletes a file. */
    SYS_open,                   /* Opens a file. */
    SYS_filesize,               /* Obtains a file's size. */
    SYS_read,                   /* Reads from a file. */
    SYS_write,                  /* Writes to a file. */
    SYS_seek,                   /* Change position in a file. */
    SYS_tell,                   /* Report current position in a file. */
    SYS_close,                  /* Closes a file. */

    /* Project 3 and optionally project 4. */
    SYS_mmap,                   /* Maps a file into memory. */
    SYS_munmap,                 /* Removes a memory mapping. */

    /* Project 4 only. */
    SYS_chdir,                  /* Changes the current directory. */
    SYS_mkdir,                  /* Create a directory. */
    SYS_lsdir                   /* Lists the current directory to stdout. */
  };

#endif /* lib/syscall-nr.h */
