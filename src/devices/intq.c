#include "devices/intq.h"
#include <debug.h>
#include "threads/thread.h"

static int next (int pos);
static bool owned_by_current_thread (const struct intq *);
static void wait (struct intq *q, struct thread **waiter);
static void signal (struct intq *q, struct thread **waiter);

/* Initializes interrupt queue Q, naming it NAME (for debugging
   purposes). */
void
intq_init (struct intq *q, const char *name) 
{
  lock_init (&q->lock, name);
  q->not_full = q->not_empty = NULL;
  q->head = q->tail = 0;
}

/* Locks out other threads from Q (with Q's lock) and interrupt
   handlers (by disabling interrupts). */
void
intq_lock (struct intq *q) 
{
  ASSERT (!intr_context ());
  ASSERT (!owned_by_current_thread (q));

  lock_acquire (&q->lock);
  q->old_level = intr_disable ();
}

/* Unlocks Q. */
void
intq_unlock (struct intq *q) 
{
  ASSERT (!intr_context ());
  ASSERT (owned_by_current_thread (q));

  lock_release (&q->lock);
  intr_set_level (q->old_level);
}

/* Returns true if Q is empty, false otherwise. */
bool
intq_empty (const struct intq *q) 
{
  ASSERT (intr_get_level () == INTR_OFF);
  return q->head == q->tail;
}

/* Returns true if Q is full, false otherwise. */
bool
intq_full (const struct intq *q) 
{
  ASSERT (intr_get_level () == INTR_OFF);
  return next (q->head) == q->tail;
}

/* Removes a byte from Q and returns it.
   Q must not be empty if called from an interrupt handler.
   Otherwise, if Q is empty, first waits until a byte is added.
   Either Q must be locked or we must be in an interrupt
   handler. */   
uint8_t
intq_getc (struct intq *q) 
{
  uint8_t byte;
  
  ASSERT (owned_by_current_thread (q));
  while (intq_empty (q))
    wait (q, &q->not_empty);

  byte = q->buf[q->tail];
  q->tail = next (q->tail);
  signal (q, &q->not_full);
  return byte;
}

/* Adds BYTE to the end of Q.
   Q must not be full if called from an interrupt handler.
   Otherwise, if Q is full, first waits until a byte is removed.
   Either Q must be locked or we must be in an interrupt
   handler. */   
void
intq_putc (struct intq *q, uint8_t byte) 
{
  ASSERT (owned_by_current_thread (q));
  while (intq_full (q))
    wait (q, &q->not_full);

  q->buf[q->head] = byte;
  q->head = next (q->head);
  signal (q, &q->not_empty);
}

/* Returns the position after POS within an intq. */
static int
next (int pos) 
{
  return (pos + 1) % INTQ_BUFSIZE;
}

/* Returns true if Q is "owned by" the current thread; that is,
   if Q is locked by the current thread or if we're in an
   external interrupt handler. */
static bool
owned_by_current_thread (const struct intq *q) 
{
  return (intr_context ()
          || (lock_held_by_current_thread (&q->lock)
              && intr_get_level () == INTR_OFF));
}

/* WAITER must be the address of Q's not_empty or not_full
   member.  Waits until the given condition is true. */
static void
wait (struct intq *q, struct thread **waiter) 
{
  ASSERT (!intr_context ());
  ASSERT (owned_by_current_thread (q));
  ASSERT ((waiter == &q->not_empty && intq_empty (q))
          || (waiter == &q->not_full && intq_full (q)));

  *waiter = thread_current ();
  thread_block ();
}

/* WAITER must be the address of Q's not_empty or not_full
   member, and the associated condition must be true.  If a
   thread is waiting for the condition, wakes it up and resets
   the waiting thread. */
static void
signal (struct intq *q, struct thread **waiter) 
{
  ASSERT (owned_by_current_thread (q));
  ASSERT ((waiter == &q->not_empty && !intq_empty (q))
          || (waiter == &q->not_full && !intq_full (q)));

  if (*waiter != NULL) 
    {
      thread_unblock (*waiter);
      *waiter = NULL;
    }
}
