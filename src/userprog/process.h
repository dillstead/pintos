#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"

tid_t process_execute (const char *filename);
void process_exit (void);
void process_activate (void);

#endif /* userprog/process.h */
