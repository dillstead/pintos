#ifndef HEADER_THREAD_H
#define HEADER_THREAD_H 1

#include <debug.h>
#include <list.h>
#include <stdint.h>

#ifdef USERPROG
#include "userprog/addrspace.h"
#endif

/* States in a thread's life cycle. */
enum thread_status
  {
    THREAD_RUNNING,     /* Running thread. */
    THREAD_READY,       /* Not running but ready to run. */
    THREAD_BLOCKED,     /* Waiting for an event to trigger. */
    THREAD_DYING        /* About to be destroyed. */
  };

/* A kernel thread or user process.

   Each thread structure is stored in its own 4 kB page.  The
   thread structure itself sits at the very bottom of the page
   (at offset 0).  The rest of the page is reserved for the
   thread's kernel stack, which grows downward from the top of
   the page (at offset 4 kB).  Here's an illustration:

        4 kB +---------------------------------+
             |          kernel stack           |
             |                |                |
             |                |                |
             |                V                |
             |         grows downward          |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             +---------------------------------+
             |             magic               |
             |               :                 |
             |               :                 |
             |              name               |
             |             status              |
        0 kB +---------------------------------+

   The upshot of this is twofold:

      1. First, `struct thread' must not be allowed to grow too
         big.  If it does, then there will not be enough room for
         the kernel stack.  Our base `struct thread' is only a
         few bytes in size.  It probably should stay well under 1
         kB.

      2. Second, kernel stacks must not be allowed to grow too
         large.  If a stack overflows, it will corrupt the thread
         state.  Thus, kernel functions should not allocate large
         structures or arrays as non-static local variables.  Use
         dynamic allocation with malloc() or palloc_get()
         instead.

   The first symptom of either of these problems will probably be
   an assertion failure in thread_current(), which checks that
   the `magic' member of the running thread's `struct thread' is
   set to THREAD_MAGIC.  Stack overflow will normally change this
   value, triggering the assertion. */
/* The `elem' member has a dual purpose.  It can be an element in
   the run queue (thread.c), or it can be an element in a
   semaphore wait list (synch.c).  It can be used these two ways
   only because they are mutually exclusive: only a thread in the
   ready state is on the run queue, whereas only a thread in the
   blocked state is on a semaphore wait list. */
struct thread
  {
    /* These members are owned by the thread_*() functions. */
    enum thread_status status;          /* Thread state. */
    char name[16];                      /* Name (for debugging purposes). */
    uint8_t *stack;                     /* Saved stack pointer. */
    list_elem elem;                     /* List element. */

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

void thread_unblock (struct thread *);
const char *thread_name (struct thread *);

struct thread *thread_current (void);
void thread_exit (void) NO_RETURN;
void thread_yield (void);
void thread_block (void);

#endif /* thread.h */
