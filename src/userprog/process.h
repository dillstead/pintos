#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"

tid_t process_execute (const char *filename);
void process_destroy (struct thread *);
void process_activate (void);

#endif /* userprog/process.h */
