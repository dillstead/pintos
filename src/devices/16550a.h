#ifndef DEVICES_16550A_H
#define DEVICES_16550A_H

#include <debug.h>
#include <stdbool.h>
#include <stdint.h>

/* Register definitions for the 16550A UART used in PCs.  This is
   a full definition of all the registers and their bits.  We
   only use a few of them, however.  */
   
/* I/O port base address for the first serial port. */
#define IO_BASE 0x3f8

/* DLAB=0 registers. */
#define RBR_REG (IO_BASE + 0)   /* Receiver Buffer Reg. (read-only). */
#define THR_REG (IO_BASE + 0)   /* Transmitter Holding Reg. (write-only). */
#define IER_REG (IO_BASE + 1)   /* Interrupt Enable Reg. (read-only). */
#define IIR_REG (IO_BASE + 2)   /* Interrupt Ident. Reg. (read-only). */
#define FCR_REG (IO_BASE + 2)   /* FIFO Control Reg. (write-only). */
#define LCR_REG (IO_BASE + 3)   /* Line Control Register. */
#define MCR_REG (IO_BASE + 4)   /* MODEM Control Register. */
#define LSR_REG (IO_BASE + 5)   /* Line Status Register (read-only). */
#define MSR_REG (IO_BASE + 6)   /* MODEM Status Register. */
#define SCR_REG (IO_BASE + 7)   /* Scratch Register. */

/* DLAB=1 registers. */
#define LS_REG (IO_BASE + 0)    /* Divisor Latch (LSB). */
#define MS_REG (IO_BASE + 1)    /* Divisor Latch (MSB). */

/* Interrupt Enable Register bits. */
#define IER_RECV 0x01           /* Enable interrupt when data received. */
#define IER_XMIT 0x02           /* Interrupt when transmit finishes. */
#define IER_LSR  0x04           /* Interrupt when LSR changes. */
#define IER_MSR  0x08           /* Interrupt when MSR changes. */

/* Interrupt Identification Register bits. */
#define IIR_PENDING 0x01        /* 0=interrupt pending. */
#define IIR_ID(IIR) (((IIR) >> 1) & 7)      /* Interrupt ID. */
#define IIR_FIFO 0xc0           /* Set when FIFO is enabled. */

/* FIFO Control Register bits. */
#define FCR_FIFO 0x01           /* 1=Enable FIFOs. */
#define FCR_RECV_RESET 0x02     /* 1=Reset receive FIFO. */
#define FCR_XMIT_RESET 0x04     /* 1=Reset transmit FIFO. */
#define FCR_DMA_MODE 0x08       /* 0=DMA mode 0, 1=DMA mode 1. */
#define FCR_FIFO_TRIGGER 0xc0   /* Mask for FIFO trigger level. */

/* Line Control Register bits. */
enum parity_type
  {
    NONE,               /* No parity bit. */
    ODD,                /* Odd parity. */
    EVEN,               /* Even parity. */
    MARK,               /* Parity bit set to 1. */
    SPACE               /* Parity bit set to 0. */
  };

static inline uint8_t
make_lcr (int bits, enum parity_type parity, int stop, bool send_break,
          bool dlab) 
{
  uint8_t lcr = 0;

  ASSERT (bits >= 5 && bits <= 8);
  switch (bits) 
    {
    case 5: lcr |= 0x00; break;
    case 6: lcr |= 0x01; break;
    case 7: lcr |= 0x02; break;
    case 8: lcr |= 0x03; break;
    }

  switch (parity)
    { 
    case NONE: lcr |= 0x00; break;
    case ODD: lcr |= 0x08; break;
    case EVEN: lcr |= 0x18; break;
    case MARK: lcr |= 0x28; break;
    case SPACE: lcr |= 0x38; break;
    default: PANIC ("bad parity %d", (int) parity); 
    }

  ASSERT (stop == 1 || stop == 2);
  if (stop > 1)
    lcr |= 0x04;

  if (send_break)
    lcr |= 0x40;
  if (dlab)
    lcr |= 0x80;

  return lcr;
}

/* MODEM Control Register. */
#define MCR_DTR 0x01            /* Data Terminal Ready. */
#define MCR_RTS 0x02            /* Request to Send. */
#define MCR_OUT1 0x04           /* Output line 1. */
#define MCR_OUT2 0x08           /* Output line 2. */
#define MCR_LOOP 0x10           /* Loopback enable. */

/* Line Status Register. */
#define LSR_DR 0x01             /* Data Ready. */
#define LSR_OE 0x02             /* Overrun Error. */
#define LSR_PE 0x04             /* Parity Error. */
#define LSR_FE 0x08             /* Framing Error. */
#define LSR_BI 0x10             /* Break Interrupt. */
#define LSR_THRE 0x20           /* THR Empty. */
#define LSR_TEMT 0x40           /* Transmitter Empty. */
#define LSR_ERF 0x80            /* Error in Receiver FIFO. */

/* MODEM Status Register. */
#define MSR_DCTS 0x01           /* Delta Clear to Send. */
#define MSR_DDSR 0x02           /* Delta Data Set Ready. */
#define MSR_TERI 0x04           /* Trailing Edge Ring Indicator. */
#define MSR_DDCD 0x08           /* Delta Data Carrier Detect. */
#define MSR_CTS  0x10           /* Clear To Send. */
#define MSR_DSR  0x20           /* Data Set Ready. */
#define MSR_RI   0x40           /* Ring Indicator. */
#define MSR_DCD  0x80           /* Data Carrier Detect. */

#endif /* devices/16550a.h */
