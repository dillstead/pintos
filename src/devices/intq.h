#ifndef DEVICES_INTQ_H
#define DEVICES_INTQ_H

#include "threads/interrupt.h"
#include "threads/synch.h"

/* An "interrupt queue", a circular buffer shared between
   kernel threads and external interrupt handlers.

   A kernel thread that touches an interrupt queue must bracket
   its accesses with calls to intq_lock() and intq_unlock().
   These functions take a lock associated with the queue (which
   locks out other kernel threads) and disable interrupts (which
   locks out interrupt handlers).

   An external interrupt handler that touches an interrupt queue
   need not take any special precautions.  Interrupts are
   disabled in an external interrupt handler, so other code will
   not interfere.  The interrupt cannot occur during an update to
   the interrupt queue by a kernel thread because kernel threads
   disable interrupts while touching interrupt queues.

   Incidentally, this has the structure of a "monitor".  Normally
   we'd use locks and condition variables from threads/synch.h to
   implement a monitor.  Unfortunately, those are intended only
   to protect kernel threads from one another, not from interrupt
   handlers. */

/* Queue buffer size, in bytes. */
#define INTQ_BUFSIZE 8

/* A circular queue of bytes. */
struct intq
  {
    /* Mutual exclusion. */
    enum intr_level old_level;  /* Excludes interrupt handlers. */
    struct lock lock;           /* Excludes kernel threads. */

    /* Waiting threads. */
    struct thread *not_full;    /* Thread waiting for not-full condition. */
    struct thread *not_empty;   /* Thread waiting for not-empty condition. */

    /* Queue. */
    uint8_t buf[INTQ_BUFSIZE];  /* Buffer. */
    int head;                   /* New data is written here. */
    int tail;                   /* Old data is read here. */
  };

void intq_init (struct intq *, const char *);
void intq_lock (struct intq *);
void intq_unlock (struct intq *);
bool intq_empty (const struct intq *);
bool intq_full (const struct intq *);
uint8_t intq_getc (struct intq *);
void intq_putc (struct intq *, uint8_t);

#endif /* devices/intq.h */
