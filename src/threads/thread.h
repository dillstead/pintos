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
    enum thread_status status;          /* Thread state. */
    char name[16];                      /* Name (for debugging purposes). */
    uint8_t *stack;                     /* Saved stack pointer. */
    list_elem rq_elem;                  /* Run queue list element. */
#ifdef USERPROG
    struct addrspace addrspace;         /* Userland address space. */
#endif
    unsigned magic;                     /* Always set to THREAD_MAGIC. */
  };

void thread_init (void);
void thread_start (void) NO_RETURN;

struct thread *thread_create (const char *name, void (*) (void *aux), void *);
#ifdef USERPROG
bool thread_execute (const char *filename);
#endif

void thread_wake (struct thread *);
const char *thread_name (struct thread *);

struct thread *thread_current (void);
void thread_exit (void) NO_RETURN;
void thread_yield (void);
void thread_sleep (void);

#endif /* thread.h */
