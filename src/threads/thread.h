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
    /* These members are owned by the thread_*() functions. */
    enum thread_status status;          /* Thread state. */
    char name[16];                      /* Name (for debugging purposes). */
    uint8_t *stack;                     /* Saved stack pointer. */
    list_elem rq_elem;                  /* Run queue list element. */

#ifdef USERPROG
    /* These members are owned by the addrspace_*() functions. */
    uint32_t *pagedir;                  /* Page directory. */
#endif
    
    /* Marker to detect stack overflow. */
    unsigned magic;                     /* Always set to THREAD_MAGIC. */
  };

void thread_init (void);
void thread_start (void);

typedef void thread_func (void *aux);
struct thread *thread_create (const char *name, thread_func *, void *);
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
