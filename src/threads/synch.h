#ifndef THREADS_SYNCH_H
#define THREADS_SYNCH_H

#include <list.h>
#include <stdbool.h>

/* A counting semaphore. */
struct semaphore 
  {
    char name[16];              /* Name (for debugging purposes only). */
    unsigned value;             /* Current value. */
    struct list waiters;        /* List of waiting threads. */
  };

void sema_init (struct semaphore *, unsigned value, const char *);
void sema_down (struct semaphore *);
void sema_up (struct semaphore *);
const char *sema_name (const struct semaphore *);
void sema_self_test (void);

/* Lock. */
struct lock 
  {
    struct thread *holder;      /* Thread holding lock (for debugging). */
    struct semaphore semaphore; /* Binary semaphore controlling access. */
  };

void lock_init (struct lock *, const char *);
void lock_acquire (struct lock *);
void lock_release (struct lock *);
bool lock_held_by_current_thread (const struct lock *);
const char *lock_name (const struct lock *);

/* Condition variable. */
struct condition 
  {
    char name[16];              /* Name (for debugging purposes only). */
    struct list waiters;        /* List of waiting threads. */
  };

void cond_init (struct condition *, const char *);
void cond_wait (struct condition *, struct lock *);
void cond_signal (struct condition *, struct lock *);
void cond_broadcast (struct condition *, struct lock *);
const char *cond_name (const struct condition *);

#endif /* threads/synch.h */
