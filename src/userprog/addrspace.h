#ifndef HEADER_ADDRSPACE_H
#define HEADER_ADDRSPACE_H 1

#include <stdbool.h>

struct thread;
bool addrspace_load (struct thread *, const char *, void (**start) (void));
void addrspace_destroy (struct thread *);
void addrspace_activate (struct thread *);

#endif /* addrspace.h */
