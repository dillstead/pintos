#ifndef HEADER_THREAD_H
#define HEADER_THREAD_H 1

#include <stdint.h>
#include "debug.h"
#include "list.h"

#ifdef USERPROG
#include "addrspace.h"
#endif

enum thread_status 
  {
    THREAD_RUNNING,
    THREAD_READY,
    THREAD_BLOCKED,
    THREAD_DYING
  };

struct thread 
  {
    enum thread_status status;
    char name[16];
    uint8_t *stack;
    list_elem rq_elem;
#ifdef USERPROG
    struct addrspace addrspace;
#endif
  };

void thread_init (const char *name, void (*) (void *aux), void *) NO_RETURN;
struct thread *thread_create (const char *name, void (*) (void *aux), void *);
void thread_destroy (struct thread *);
struct thread *thread_current (void);

#ifdef USERPROG
bool thread_execute (const char *filename);
#endif

void thread_ready (struct thread *);
void thread_exit (void) NO_RETURN;

void thread_yield (void);
void thread_sleep (void);

#endif /* thread.h */
