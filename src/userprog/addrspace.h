#ifndef USERPROG_ADDRSPACE_H
#define USERPROG_ADDRSPACE_H

#include <stdbool.h>

struct thread;
bool addrspace_load (struct thread *, const char *, void (**start) (void));
void addrspace_destroy (struct thread *);
void addrspace_activate (struct thread *);

#endif /* userprog/addrspace.h */
