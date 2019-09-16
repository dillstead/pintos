#ifndef THREADS_THREAD_H
#define THREADS_THREAD_H

#include <debug.h>
#include <list.h>
#include <stdint.h>
#include "threads/synch.h"

/* States in a thread's life cycle. */
enum thread_status
  {
    THREAD_RUNNING,     /* Running thread. */
    THREAD_READY,       /* Not running but ready to run. */
    THREAD_BLOCKED,     /* Waiting for an event to trigger. */
    THREAD_DYING,       /* About to be destroyed. */
 #ifdef USERPROG    
    THREAD_EXITING      /* Exiting, to be freed by parent. */
 #endif
  };

/* Thread identifier type.
   You can redefine this to whatever type you like. */
typedef int tid_t;
#ifdef USERPROG
#define TID_NONE ((tid_t) 0)            /* Identifiers start at 1. */
#endif
#define TID_ERROR ((tid_t) -1)          /* Error value for tid_t. */

/* Thread priorities. */
#define PRI_MIN 0                       /* Lowest priority. */
#define PRI_DEFAULT 31                  /* Default priority. */
#define PRI_MAX 63                      /* Highest priority. */

/* Thread niceness. */
#define NICE_MIN -20
#define NICE_DEFAULT 0
#define NICE_MAX 20

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
             |              magic              |
             |                :                |
             |                :                |
             |               name              |
             |              status             |
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
         dynamic allocation with malloc() or palloc_get_page()
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
    /* Owned by thread.c. */
    tid_t tid;                          /* Thread identifier. */
    enum thread_status status;          /* Thread state. */
    char name[16];                      /* Name (for debugging purposes). */
    uint8_t *stack;                     /* Saved stack pointer. */
    int priority;                       /* Effective Priority (can be raised 
                                           or lowered automatically by 
                                           priority donation). */
    int base_priority;                  /* The effective priority of the 
                                           thread can't go lower than the 
                                           base */
    struct list_elem allelem;           /* List element for all threads list. */

    /* Shared between thread.c and synch.c. */
    struct list_elem elem;              /* List element. */

    /* List of locks the thread is currently holding. */ 
    struct list locks_owned_list;

    /* If the thread is waiting on a lock, the lock it's waiting on. */
    struct lock *waiting_lock;

    /* Used for advanced scheduler. */

    int nice;                           /* Only used with the advanced 
                                           scheduler.  How "nice" a thread
                                           is to other threads.  A postive
                                           value decreases the priority of 
                                           the thread while a negative value
                                           increases the priority. */
    int recent_cpu;                     /* Estimate of how much CPU the thread
                                           has used recently. */

 #ifdef USERPROG
    /* Owned by userprog/process.c. */
    uint32_t *pagedir;                  /* Page directory. */

    /* Parent thread identifier, TID_NONE if the thread has no parent
       or the parent has already exited. */
    tid_t ptid;

    /* List of child threads. */
    struct list child_list;

    /* List element for child list. */
    struct list_elem child_elem;

    /* Exit status. */
    int exit_status;
    
    /* Lock used by process_exit() and process_wait(). */
    struct lock exit_lock;

    /* Protected by exit_lock and signaled when the process is exiting. */
    struct condition exiting;

    /* Table of open files. */
    struct file **ofiles;

    /* Table of memory mapped files. */
    struct mmap *mfiles;

    /* User stack pointer used for dynamic stack growth. */
    void *user_esp;
#endif

    /* Owned by thread.c. */
    unsigned magic;                     /* Detects stack overflow. */
  };

/* If false (default), use round-robin scheduler.
   If true, use multi-level feedback queue scheduler.
   Controlled by kernel command-line option "-o mlfqs". */
extern bool thread_mlfqs;

void thread_init (void);
void thread_start (void);

void thread_tick (void);
void thread_print_stats (void);

typedef void thread_func (void *aux);
tid_t thread_create (const char *name, int priority, thread_func *, void *);

void thread_block (void);
void thread_sleep (int64_t ticks);
void thread_unblock (struct thread *);

struct thread *thread_current (void);
tid_t thread_tid (void);
const char *thread_name (void);

void thread_exit (void) NO_RETURN;
void thread_yield (void);

/* Performs some operation on thread t, given auxiliary data AUX. */
typedef void thread_action_func (struct thread *t, void *aux);
void thread_foreach (thread_action_func *, void *);

int thread_get_priority (void);
void thread_set_priority (int);

/* For internal use to support priority donation. */
void thread_lock_acquired (struct lock *lock);
void thread_lock_will_wait (struct lock *lock);
void thread_lock_released (struct lock *lock);

/* Used for advanced scheduler. */
int thread_get_nice (void);
void thread_set_nice (int);
int thread_get_recent_cpu (void);
int thread_get_load_avg (void);

#endif /* threads/thread.h */
