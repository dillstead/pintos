#ifndef HEADER_THREAD_H
#define HEADER_THREAD_H 1

#include <stdint.h>
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
    uint32_t *stack;
    list_elem rq_elem;
#ifdef USERPROG
    struct addrspace addrspace;
#endif
  };

void thread_init (void);

struct thread *thread_create (const char *name,
                              void (*function) (void *aux), void *aux);
void thread_destroy (struct thread *);
struct thread *thread_current (void);

#ifdef USERPROG
void thread_execute (const char *filename);
#endif

void thread_start (struct thread *);
void thread_ready (struct thread *);
void thread_exit (void);

void thread_yield (void);
void thread_sleep (void);
void thread_schedule (void);

void thread_self_test (void);

#endif /* thread.h */
