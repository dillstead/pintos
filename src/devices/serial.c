#include "devices/serial.h"
#include <debug.h>
#include "devices/16550a.h"
#include "devices/timer.h"
#include "threads/io.h"
#include "threads/interrupt.h"
#include "threads/synch.h"
#include "threads/thread.h"

#define QUEUE_BUFSIZE 8

struct queue 
  {
    enum intr_level old_level;
    struct lock lock;
    
    struct thread *not_full, *not_empty;
    uint8_t buf[QUEUE_BUFSIZE];
    int head, tail;
  };

static struct queue recv_queue, xmit_queue;
static bool polled_io;

static void set_serial (int bps, int bits, enum parity_type parity, int stop);
static void update_ier (void);
static intr_handler_func serial_interrupt;

static void queue_init (struct queue *, const char *);
static void queue_lock (struct queue *);
static void queue_unlock (struct queue *);
static bool queue_empty (const struct queue *);
static bool queue_full (const struct queue *);
static uint8_t queue_getc (struct queue *);
static void queue_putc (struct queue *, uint8_t);

/* Initializes the serial port device for polling mode.
   Polling mode busy-waits for the serial port to become free
   before writing to it.  It's slow, but until interrupts have
   been initialized it's all we can do. */
void
serial_init_poll (void) 
{
  ASSERT (!polled_io);
  polled_io = true;
  outb (IER_REG, 0);                    /* Turn off all interrupts. */
  outb (FCR_REG, 0);                    /* Disable FIFO. */
  set_serial (9600, 8, NONE, 1);        /* 9600 bps, N-8-1. */
  outb (MCR_REG, 8);                    /* Turn on OUT2 output line. */
} 

/* Initializes the serial port device for queued interrupt-driven
   I/O.  With interrupt-driven I/O we don't waste CPU time
   waiting for the serial device to become ready. */
void
serial_init_queue (void) 
{
  ASSERT (polled_io);
  polled_io = false;
  queue_init (&recv_queue, "serial recv");
  queue_init (&xmit_queue, "serial xmit");
  intr_register (0x20 + 4, 0, INTR_OFF, serial_interrupt, "serial");
}

static void putc_poll (uint8_t);
static void putc_queue (uint8_t);

/* Sends BYTE to the serial port. */
void
serial_putc (uint8_t byte) 
{
  if (polled_io || intr_context ())
    putc_poll (byte);
  else
    putc_queue (byte); 
}

static void
putc_poll (uint8_t byte)
{
  while ((inb (LSR_REG) & LSR_THRE) == 0)
    continue;
  outb (THR_REG, byte);
}

static void
putc_queue (uint8_t byte)
{
  queue_lock (&xmit_queue);

  queue_putc (&xmit_queue, byte); 
  update_ier ();

  queue_unlock (&xmit_queue);
}

/* Reads and returns a byte from the serial port. */
uint8_t
serial_getc (void) 
{
  uint8_t byte;
  queue_lock (&recv_queue);

  /* Read a byte. */
  byte = queue_getc (&recv_queue);
  update_ier ();

  queue_unlock (&recv_queue);
  return byte;
}

/* Configures the first serial port for BPS bits per second,
   BITS bits per byte, the given PARITY, and STOP stop bits. */
static void
set_serial (int bps, int bits, enum parity_type parity, int stop)
{
  int baud_base = 1843200 / 16;         /* Base rate of 16550A. */
  uint16_t divisor = baud_base / bps;   /* Clock rate divisor. */

  /* Enable DLAB. */
  outb (LCR_REG, make_lcr (bits, parity, stop, false, true));

  /* Set baud rate. */
  outb (LS_REG, divisor & 0xff);
  outb (MS_REG, divisor >> 8);
  
  /* Reset DLAB. */
  outb (LCR_REG, make_lcr (bits, parity, stop, false, false));
}

/* Update interrupt enable register.
   If our transmit queue is empty, turn off transmit interrupt.
   If our receive queue is full, turn off receive interrupt. */
static void
update_ier (void) 
{
  uint8_t ier = 0;
  
  ASSERT (intr_get_level () == INTR_OFF);

  if (!queue_empty (&xmit_queue))
    ier |= IER_XMIT;
  if (!queue_full (&recv_queue))
    ier |= IER_RECV;
  outb (IER_REG, ier);
}

static void
serial_interrupt (struct intr_frame *f UNUSED) 
{
  /* As long as we have space in our buffer for a byte,
     and the hardware has a byte to give us,
     receive a byte. */
  while (!queue_full (&recv_queue) && (inb (LSR_REG) & LSR_DR) != 0)
    {
      uint8_t byte = inb (RBR_REG);
      //outb (0x0402, byte);
      queue_putc (&recv_queue, byte); 
    }
  
  /* As long as we have a byte to transmit,
     and the hardware is ready to accept a byte for transmission,
     transmit a byte. */
  while (!queue_empty (&xmit_queue) && (inb (LSR_REG) & LSR_THRE) != 0) 
    outb (THR_REG, queue_getc (&xmit_queue));
  
  /* Update interrupt enable register based on queue status. */
  update_ier ();
}

static int next (int pos);
static bool owned_by_current_thread (const struct queue *);
static void wait (struct queue *q, struct thread **waiter);
static void signal (struct queue *q, struct thread **waiter);

static void
queue_init (struct queue *q, const char *name) 
{
  lock_init (&q->lock, name);
  q->not_full = q->not_empty = NULL;
  q->head = q->tail = 0;
}

static void
queue_lock (struct queue *q) 
{
  lock_acquire (&q->lock);
  q->old_level = intr_disable ();
}

static void
queue_unlock (struct queue *q) 
{
  ASSERT (!intr_context ());
  ASSERT (owned_by_current_thread (q));

  lock_release (&q->lock);
  intr_set_level (q->old_level);
}

static bool
queue_empty (const struct queue *q) 
{
  ASSERT (intr_get_level () == INTR_OFF);
  return q->head == q->tail;
}

static bool
queue_full (const struct queue *q) 
{
  ASSERT (intr_get_level () == INTR_OFF);
  return next (q->head) == q->tail;
}

static uint8_t
queue_getc (struct queue *q) 
{
  uint8_t byte;
  
  ASSERT (owned_by_current_thread (q));
  while (queue_empty (q))
    wait (q, &q->not_empty);

  byte = q->buf[q->tail];
  q->tail = next (q->tail);
  signal (q, &q->not_full);
  return byte;
}

static void
queue_putc (struct queue *q, uint8_t byte) 
{
  ASSERT (owned_by_current_thread (q));
  while (queue_full (q))
    wait (q, &q->not_full);

  q->buf[q->head] = byte;
  q->head = next (q->head);
  signal (q, &q->not_empty);
}

static int
next (int pos) 
{
  return (pos + 1) % QUEUE_BUFSIZE;
}

static bool
owned_by_current_thread (const struct queue *q) 
{
  return (intr_context ()
          || (lock_held_by_current_thread (&q->lock)
              && intr_get_level () == INTR_OFF));
}

static void
wait (struct queue *q, struct thread **waiter) 
{
  ASSERT (!intr_context ());
  ASSERT (owned_by_current_thread (q));

  *waiter = thread_current ();
  thread_block ();
}

static void
signal (struct queue *q, struct thread **waiter) 
{
  ASSERT (owned_by_current_thread (q));

  if (*waiter != NULL) 
    {
      thread_unblock (*waiter);
      *waiter = NULL;
    }
}
