#include "devices/serial.h"
#include <debug.h>
#include "devices/16550a.h"
#include "devices/intq.h"
#include "devices/timer.h"
#include "threads/io.h"
#include "threads/interrupt.h"
#include "threads/synch.h"
#include "threads/thread.h"

/* Transmission mode. */
static enum { UNINIT, POLL, QUEUE } mode;

/* Data to be transmitted. */
static struct intq txq;

static void set_serial (int bps, int bits, enum parity_type parity, int stop);
static void write_ier (void);
static intr_handler_func serial_interrupt;

/* Initializes the serial port device for polling mode.
   Polling mode busy-waits for the serial port to become free
   before writing to it.  It's slow, but until interrupts have
   been initialized it's all we can do. */
void
serial_init_poll (void) 
{
  ASSERT (mode == UNINIT);
  outb (IER_REG, 0);                    /* Turn off all interrupts. */
  outb (FCR_REG, 0);                    /* Disable FIFO. */
  set_serial (9600, 8, NONE, 1);        /* 9600 bps, N-8-1. */
  outb (MCR_REG, 8);                    /* Turn on OUT2 output line. */
  mode = POLL;
} 

/* Initializes the serial port device for queued interrupt-driven
   I/O.  With interrupt-driven I/O we don't waste CPU time
   waiting for the serial device to become ready. */
void
serial_init_queue (void) 
{
  ASSERT (mode == POLL);
  intq_init (&txq, "serial xmit");
  intr_register (0x20 + 4, 0, INTR_OFF, serial_interrupt, "serial");
  mode = QUEUE;
}

/* Sends BYTE to the serial port. */
void
serial_putc (uint8_t byte) 
{
  if (mode == POLL || intr_context ())
    {
      /* Poll the serial port until it's ready for a byte, and
         then transmit. */
      while ((inb (LSR_REG) & LSR_THRE) == 0)
        continue;
      outb (THR_REG, byte);
    }
  else
    {
      /* Lock the queue, add a byte, and update the interrupt
         enable register. */
      intq_lock (&txq);
      intq_putc (&txq, byte); 
      write_ier ();
      intq_unlock (&txq);
    }
}

/* Configures the serial port for BPS bits per second, BITS bits
   per byte, the given PARITY, and STOP stop bits. */
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
   If our transmit queue is empty, turn off transmit interrupt. */
static void
write_ier (void) 
{
  outb (IER_REG, intq_empty (&txq) ? 0 : IER_XMIT);
}

/* Serial interrupt handler.
   As long as we have a byte to transmit,
   and the hardware is ready to accept a byte for transmission,
   transmit a byte.
   Then update interrupt enable register based on queue
   status. */
static void
serial_interrupt (struct intr_frame *f UNUSED) 
{
  while (!intq_empty (&txq) && (inb (LSR_REG) & LSR_THRE) != 0) 
    outb (THR_REG, intq_getc (&txq));
  write_ier ();
}
