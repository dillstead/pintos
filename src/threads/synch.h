#ifndef HEADER_SYNCH_H
#define HEADER_SYNCH_H 1

#include <stdbool.h>
#include "list.h"

struct semaphore 
  {
    char name[16];
    unsigned value;
    struct list waiters;
  };

void sema_init (struct semaphore *, unsigned value, const char *);
void sema_down (struct semaphore *);
void sema_up (struct semaphore *);
const char *sema_name (const struct semaphore *);
void sema_self_test (void);

struct lock 
  {
    char name[16];
    struct thread *holder;
    struct semaphore semaphore;
  };

void lock_init (struct lock *, const char *);
void lock_acquire (struct lock *);
void lock_release (struct lock *);
bool lock_held_by_current_thread (const struct lock *);
const char *lock_name (const struct lock *);

struct condition 
  {
    char name[16];
    struct list waiters;
  };

void cond_init (struct condition *, const char *);
void cond_wait (struct condition *, struct lock *);
void cond_signal (struct condition *, struct lock *);
void cond_broadcast (struct condition *, struct lock *);
const char *cond_name (const struct condition *);

#endif /* synch.h */
