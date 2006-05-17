#ifndef __LIB_SYSCALL_NR_H
#define __LIB_SYSCALL_NR_H

/* System call numbers. */
enum 
  {
    /* Projects 2 and later. */
    SYS_HALT,                   /* Halts the operating system. */
    SYS_EXIT,                   /* Terminates this process. */
    SYS_EXEC,                   /* Start another process. */
    SYS_WAIT,                   /* Waits for a child process to die. */
    SYS_CREATE,                 /* Creates a file. */
    SYS_REMOVE,                 /* Deletes a file. */
    SYS_OPEN,                   /* Opens a file. */
    SYS_FILESIZE,               /* Obtains a file's size. */
    SYS_READ,                   /* Reads from a file. */
    SYS_WRITE,                  /* Writes to a file. */
    SYS_SEEK,                   /* Change position in a file. */
    SYS_TELL,                   /* Report current position in a file. */
    SYS_CLOSE,                  /* Closes a file. */

    /* Project 3 and optionally project 4. */
    SYS_MMAP,                   /* Maps a file into memory. */
    SYS_MUNMAP,                 /* Removes a memory mapping. */

    /* Project 4 only. */
    SYS_CHDIR,                  /* Changes the current directory. */
    SYS_MKDIR,                  /* Create a directory. */
    SYS_LSDIR                   /* Lists the current directory to stdout. */
  };

#endif /* lib/syscall-nr.h */
