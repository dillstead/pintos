#ifndef USERPROG_ADDRSPACE_H
#define USERPROG_ADDRSPACE_H

#include "threads/thread.h"

tid_t addrspace_execute (const char *filename);
void addrspace_destroy (struct thread *);
void addrspace_activate (void);

#endif /* userprog/addrspace.h */
