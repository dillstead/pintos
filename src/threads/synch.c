#include "synch.h"
#include "interrupt.h"
#include "lib.h"
#include "malloc.h"
#include "thread.h"

/* One thread in a list. */
struct thread_elem
  {
    struct list_elem elem;
    struct thread *thread;      
  };

/* Initializes semaphore SEMA to VALUE and names it NAME (for
   debugging purposes only).  A semaphore is a nonnegative
   integer along with two atomic operators for manipulating it:

   - down or "P": wait for the value to become positive, then
     decrement it.

   - up or "V": increment the value (and wake up one waiting
     thread, if any). */
void
sema_init (struct semaphore *sema, unsigned value, const char *name) 
{
  ASSERT (sema != NULL);
  ASSERT (name != NULL);

  strlcpy (sema->name, name, sizeof sema->name);
  sema->value = value;
  list_init (&sema->waiters);
}

/* Waits for SEMA's value to become positive and then
   atomically decrements it.

   This function may sleep, so it must not be called within an
   interrupt handler. */
void
sema_down (struct semaphore *sema) 
{
  enum if_level old_level;

  ASSERT (sema != NULL);
  ASSERT (!intr_context ());

  old_level = intr_disable ();
  while (sema->value == 0) 
    {
      struct thread_elem te;
      te.thread = thread_current ();
      list_push_back (&sema->waiters, &te.elem);
      thread_sleep ();
    }
  sema->value--;
  intr_set_level (old_level);
}

/* Increments SEMA's value.  Wakes up one thread of those waiting
   for SEMA, if any. 

   This function may be called from an interrupt handler. */
void
sema_up (struct semaphore *sema) 
{
  enum if_level old_level;

  ASSERT (sema != NULL);

  old_level = intr_disable ();
  if (!list_empty (&sema->waiters)) 
    thread_ready (list_entry (list_pop_front (&sema->waiters),
                              struct thread_elem, elem)->thread);
  sema->value++;
  intr_set_level (old_level);
}

/* Return SEMA's name (for debugging purposes). */
const char *
sema_name (const struct semaphore *sema) 
{
  return sema->name;
}

static void
sema_test_helper (void *sema_) 
{
  struct semaphore *sema = sema_;
  int i;

  for (i = 0; i < 10; i++) 
    {
      sema_down (&sema[0]);
      sema_up (&sema[1]);
    }
}

/* Self-test for semaphores that makes control "ping-pong"
   between a pair of threads.  Insert calls to printk() to see
   what's going on. */
void
sema_self_test (void) 
{
  struct thread *thread;
  struct semaphore sema[2];
  int i;

  sema_init (&sema[0], 0, "ping");
  sema_init (&sema[1], 0, "pong");
  thread = thread_create ("sema-test", sema_test_helper, &sema);
  for (i = 0; i < 10; i++) 
    {
      sema_up (&sema[0]);
      sema_down (&sema[1]);
    }
}

/* Initializes LOCK and names it NAME (for debugging purposes).
   A lock can be held by at most a single thread at any given
   time.  Our locks are not "recursive", meaning that it is an
   error for a thread that holds a lock to attempt to reacquire
   it.

   A lock is a specialization of a semaphore with an initial
   value of 1.  The difference between a lock and such a
   semaphore is twofold.  First, a semaphore can have a value
   greater than 1, but a lock can only be owned by a single
   thread at a time.  Second, a semaphore does not have an owner,
   meaning that one thread can "down" the semaphore and then
   another one "up" it, but with a lock the same thread must both
   acquire and release it.  When these restrictions prove
   onerous, it's a good sign that a semaphore should be used,
   instead of a lock. */
void
lock_init (struct lock *lock, const char *name)
{
  ASSERT (lock != NULL);
  ASSERT (name != NULL);

  strlcpy (lock->name, name, sizeof lock->name);
  lock->holder = NULL;
  sema_init (&lock->semaphore, 1, name);
}

/* Acquires LOCK, sleeping until it becomes available if
   necessary.  The lock must not already be held by the current
   thread.

   This function may sleep, so it must not be called within an
   interrupt handler. */
void
lock_acquire (struct lock *lock)
{
  enum if_level old_level;

  ASSERT (lock != NULL);
  ASSERT (!intr_context ());
  ASSERT (!lock_held_by_current_thread (lock));

  old_level = intr_disable ();
  sema_down (&lock->semaphore);
  lock->holder = thread_current ();
  intr_set_level (old_level);
}

/* Releases LOCK, which must be owned by the current thread.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to signal a condition variable within an
   interrupt handler. */
void
lock_release (struct lock *lock) 
{
  enum if_level old_level;

  ASSERT (lock != NULL);
  ASSERT (lock_held_by_current_thread (lock));

  old_level = intr_disable ();
  lock->holder = NULL;
  sema_up (&lock->semaphore);
  intr_set_level (old_level);
}

/* Returns true if the current thread holds LOCK, false
   otherwise.  (Note that testing whether some other thread holds
   a lock would be racy.) */
bool
lock_held_by_current_thread (const struct lock *lock) 
{
  ASSERT (lock != NULL);

  return lock->holder == thread_current ();
}

/* Returns the name of LOCK (for debugging purposes). */
const char *
lock_name (const struct lock *lock) 
{
  ASSERT (lock != NULL);

  return lock->name;
}

/* One semaphore in a list. */
struct semaphore_elem 
  {
    struct list_elem elem;
    struct semaphore semaphore;
  };

/* Initializes condition variable COND and names it NAME.  A
   condition variable allows one piece of code to signal a
   condition and cooperating code to receive the signal and act
   upon it. */
void
cond_init (struct condition *cond, const char *name) 
{
  ASSERT (cond != NULL);
  ASSERT (name != NULL);

  strlcpy (cond->name, name, sizeof cond->name);
  list_init (&cond->waiters);
}

/* Atomically releases LOCK and waits for COND to be signaled by
   some other piece of code.  After COND is signalled, LOCK is
   reacquired before returning.  LOCK must be held before calling
   this function.

   The monitor implemented by this function is "Mesa" style, not
   "Hoare" style.  That is, sending a signal is not atomic with
   delivering it.  Thus, typically the caller must recheck the
   condition after the wait completes and, if necessary, wait
   again.

   A given condition variable is associated with only a single
   lock, but one lock may be be associated with any number of
   condition variables.  That is, there is a one-to-many mapping
   from locks to condition variables.

   This function may sleep, so it must not be called within an
   interrupt handler. */
void
cond_wait (struct condition *cond, struct lock *lock) 
{
  struct semaphore_elem waiter;

  ASSERT (cond != NULL);
  ASSERT (lock != NULL);
  ASSERT (!intr_context ());
  ASSERT (lock_held_by_current_thread (lock));
  
  sema_init (&waiter.semaphore, 0, "condition");
  list_push_back (&cond->waiters, &waiter.elem);
  lock_release (lock);
  sema_down (&waiter.semaphore);
  lock_acquire (lock);
}

/* If any threads are waiting on COND (protected by LOCK), then
   this function signals one of them to wake up from its wait.
   LOCK must be held before calling this function.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to signal a condition variable within an
   interrupt handler. */
void
cond_signal (struct condition *cond, struct lock *lock) 
{
  ASSERT (cond != NULL);
  ASSERT (lock != NULL);
  ASSERT (!intr_context ());
  ASSERT (lock_held_by_current_thread (lock));

  if (!list_empty (&cond->waiters)) 
    sema_up (&list_entry (list_pop_front (&cond->waiters),
                          struct semaphore_elem, elem)->semaphore);
}

/* Wakes up all threads, if any, waiting on COND (protected by
   LOCK).  LOCK must be held before calling this function.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to signal a condition variable within an
   interrupt handler. */
void
cond_broadcast (struct condition *cond, struct lock *lock) 
{
  ASSERT (cond != NULL);
  ASSERT (lock != NULL);

  while (!list_empty (&cond->waiters))
    cond_signal (cond, lock);
}

/* Returns COND's name (for debugging purposes). */
const char *
cond_name (const struct condition *cond)
{
  ASSERT (cond != NULL);

  return cond->name;
}
