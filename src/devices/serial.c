#include "serial.h"
#include "16550a.h"
#include "debug.h"
#include "io.h"
#include "timer.h"

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

void
serial_init (void) 
{
  outb (IER_REG, 0);    /* Turn off all interrupts. */
  outb (FCR_REG, 0);    /* Disable FIFO. */
  set_serial (9600, 8, NONE, 1);
  outb (MCR_REG, 0);    /* Turn off output lines. */
}

void
serial_outb (uint8_t byte) 
{
  while ((inb (LSR_REG) & LSR_THRE) == 0)
    continue;
  outb (THR_REG, byte);
}
