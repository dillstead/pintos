/**
 * Universal Host Controller Interface driver
 * TODO:
 *	Stall timeouts
 * 	Better (any) root hub handling
 */

#include <round.h>
#include <stdio.h>
#include <string.h>
#include <kernel/bitmap.h>
#include "threads/pte.h"
#include "threads/malloc.h"
#include "threads/palloc.h"
#include "threads/synch.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "devices/pci.h"
#include "devices/usb.h"
#include "devices/timer.h"

#define UHCI_MAX_PORTS		8

#define FRAME_LIST_ENTRIES	1024
#define TD_ENTRIES		(4096/32)	/* number of entries allocated */
#define QH_ENTRIES		(4096/16)

/* uhci pci registers */
#define UHCI_REG_USBCMD		0x00	/* Command */
#define UHCI_REG_USBSTS		0x02	/* Status */
#define UHCI_REG_USBINTR	0x04	/* interrupt enable */
#define UHCI_REG_FRNUM		0x06	/* frame number */
#define UHCI_REG_FLBASEADD	0x08	/* frame list base address */
#define UHCI_REG_SOFMOD		0x0c	/* start of frame modify */
#define UHCI_REG_PORTSC1	0x10	/* port 1 status/control */
#define UHCI_REG_PORTSC2	0x12	/* port 2 status/control */
#define UHCI_REGSZ		0x20	/* register iospace size */

/* in PCI config space for some reason */
#define UHCI_REG_LEGSUP		0xC0


/* command register */
#define USB_CMD_MAX_PACKET	(1 << 7)
#define USB_CMD_CONFIGURE	(1 << 6)
#define USB_CMD_FGR		(1 << 4)	/* force global resume */
#define USB_CMD_EGSM		(1 << 3)	/* global suspend mode */
#define USB_CMD_GRESET		(1 << 2)	/* global reset */
#define USB_CMD_HCRESET		(1 << 1)	/* host controller reset */
#define USB_CMD_RS		(1 << 0)	/* run/stop */

/* status register */
#define USB_STATUS_HALTED	(1 << 5)
#define USB_STATUS_PROCESSERR	(1 << 4)
#define USB_STATUS_HOSTERR	(1 << 3)
#define USB_STATUS_RESUME	(1 << 2)
#define USB_STATUS_INTERR	(1 << 1)
#define USB_STATUS_USBINT	(1 << 0)

/* interrupt enable register */
#define USB_INTR_SHORT		(1 << 3)	/* enable short packets */
#define USB_INTR_IOC		(1 << 2)	/* interrupt on complete */
#define USB_INTR_RESUME		(1 << 1)	/* resume interrupt enable */
#define USB_INTR_TIMEOUT	(1 << 0)	/* timeout int enable */

/* port control register */
#define USB_PORT_SUSPEND	(1 << 12)
#define USB_PORT_RESET		(1 << 9)
#define USB_PORT_LOWSPEED	(1 << 8)
#define USB_PORT_RESUMED	(1 << 6)	/* resume detected */
#define USB_PORT_CHANGE		(1 << 3)	/* enable change */
#define USB_PORT_ENABLE		(1 << 2)	/* enable the port */
#define USB_PORT_CONNECTCHG	(1 << 1)	/* connect status changed */
#define USB_PORT_CONNECTSTATUS	(1 << 0)	/* device is connected */

#define ptr_to_flp(x)	(((uintptr_t)x) >> 4)
#define flp_to_ptr(x)	(uintptr_t)(((uintptr_t)x) << 4)

/* frame structures */
#pragma pack(1)
struct frame_list_ptr
{
  uint32_t terminate:1;
  uint32_t qh_select:1;
  uint32_t depth_select:1;	/* only for TD */
  uint32_t resv:1;		/* zero */
  uint32_t flp:28;		/* frame list pointer */
};

struct td_token
{
  uint32_t pid:8;		/* packet id */
  uint32_t dev_addr:7;		/* device address */
  uint32_t end_point:4;
  uint32_t data_toggle:1;
  uint32_t resv:1;
  uint32_t maxlen:11;		/* maximum pkt length */
};

struct td_control
{
  uint32_t actual_len:11;
  uint32_t resv1:5;

  /* status information */
  uint32_t resv2:1;
  uint32_t bitstuff:1;
  uint32_t timeout:1;
  uint32_t nak:1;
  uint32_t babble:1;
  uint32_t buffer_error:1;
  uint32_t stalled:1;
  uint32_t active:1;

  /* config data */
  uint32_t ioc:1;		/* issue int on complete */
  uint32_t ios:1;		/* isochronous select */
  uint32_t ls:1;		/* low speed device */
  uint32_t error_limit:2;	/* errors before interrupt */
  uint32_t spd:1;		/* short packet detect */
  uint32_t resv3:2;
};

#define TD_FL_ASYNC	1
#define TD_FL_USED	0x80000000

struct tx_descriptor
{
  struct frame_list_ptr flp;
  struct td_control control;
  struct td_token token;
  uint32_t buf_ptr;


  struct frame_list_ptr *head;	/* for fast removal */
  uint32_t flags;
};

struct queue_head
{
  struct frame_list_ptr qhlp;	/* queue head link pointer */
  struct frame_list_ptr qelp;	/* queue elem link pointer */
};
#pragma pack()

struct uhci_info
{
  struct pci_dev *dev;
  struct pci_io *io;		/* pci io space */
  struct lock lock;
  struct frame_list_ptr *frame_list;	/* page aligned frame list */

  struct tx_descriptor *td_pool;
  struct bitmap *td_used;
  struct queue_head *qh_pool;
  struct bitmap *qh_used;

  uint8_t num_ports;
  uint8_t attached_ports;

  int timeouts;			/* number of timeouts */

  struct semaphore td_sem;
  struct list devices;		/* devices on host */
  struct list waiting;		/* threads waiting */
};

struct usb_wait
{
  struct tx_descriptor *td;
  struct uhci_dev_info *ud;
  struct semaphore sem;
  struct list_elem peers;
};

struct uhci_dev_info
{
  struct uhci_info *ui;		/* owner */
  bool low_speed;		/* whether device is low speed */
  int dev_addr;			/* device address */
  int errors;			/* aggregate errors */
  struct list_elem peers;	/* next dev on host */
  struct lock lock;
  struct queue_head *qh;
};

struct uhci_eop_info
{
  struct uhci_dev_info *ud;
  int eop;
  int maxpkt;			/* max packet size */
  int toggle;			/* data toggle bit for bulk transfers */
};

#define uhci_lock(x)	lock_acquire(&(x)->lock)
#define uhci_unlock(x)	lock_release(&(x)->lock)
#define dev_lock(x)	lock_acquire(&(x)->lock)
#define dev_unlock(x)	lock_release(&(x)->lock)


static int token_to_pid (int token);

static int uhci_tx_pkt (host_eop_info eop, int token, void *pkt,
			int min_sz, int max_sz, int *in_sz, bool wait);

static int uhci_detect_change (host_info);


static int uhci_tx_pkt_now (struct uhci_eop_info *ue, int token, void *pkt,
			    int sz);
static int uhci_tx_pkt_wait (struct uhci_eop_info *ue, int token, void *pkt,
			     int max_sz, int *in_sz);
static int uhci_tx_pkt_bulk (struct uhci_eop_info *ue, int token, void *buf,
			     int sz, int *tx);


static int uhci_process_completed (struct uhci_info *ui);

static struct tx_descriptor *uhci_acquire_td (struct uhci_info *);
static void uhci_release_td (struct uhci_info *, struct tx_descriptor *);
static void uhci_remove_qh (struct uhci_info *ui, struct queue_head *qh);


static void qh_free (struct uhci_info *ui, struct queue_head *qh);
static struct queue_head *qh_alloc (struct uhci_info *ui);

static struct uhci_info *uhci_create_info (struct pci_io *io);
static void uhci_destroy_info (struct uhci_info *ui);
static host_eop_info uhci_create_eop (host_dev_info hd, int eop, int maxpkt);
static void uhci_remove_eop (host_eop_info hei);
static host_dev_info uhci_create_chan (host_info hi, int dev_addr, int ver);
static void uhci_destroy_chan (host_dev_info);
static void uhci_modify_chan (host_dev_info, int dev_addr, int ver);
static void uhci_set_toggle (host_eop_info, int toggle);

static struct tx_descriptor *td_from_pool (struct uhci_info *ui, int idx);

static int check_and_flip_change (struct uhci_info *ui, int reg);
static void uhci_stop (struct uhci_info *ui);
static void uhci_run (struct uhci_info *ui);
static void uhci_stop_unlocked (struct uhci_info *ui);
static void uhci_run_unlocked (struct uhci_info *ui);
#define uhci_is_stopped(x) 	(pci_reg_read16((x)->io, UHCI_REG_USBSTS) \
				& USB_STATUS_HALTED)
#define uhci_port_enabled(x, y) (pci_reg_read16((x)->io, (y)) & USB_PORT_ENABLE)
static void uhci_add_td_to_qh (struct queue_head *qh,
			       struct tx_descriptor *td);
static void uhci_remove_error_td (struct tx_descriptor *td);
static void uhci_setup_td (struct tx_descriptor *td, int dev_addr, int token,
			   int eop, void *pkt, int sz, int toggle, bool ls);
static int uhci_enable_port (struct uhci_info *ui, int port);
static void uhci_irq (void *uhci_data);
static void uhci_detect_ports (struct uhci_info *ui);

static int uhci_remove_stalled (struct uhci_info *ui);
static void uhci_stall_watchdog (struct uhci_info *ui);

static void dump_all_qh (struct uhci_info *ui);
static void dump_qh (struct queue_head *qh);

static void dump_regs (struct uhci_info *ui);

void uhci_init (void);


static struct usb_host uhci_host = {
  .name = "UHCI",
  .tx_pkt = uhci_tx_pkt,
  .detect_change = uhci_detect_change,
  .create_dev_channel = uhci_create_chan,
  .remove_dev_channel = uhci_destroy_chan,
  .modify_dev_channel = uhci_modify_chan,
  .set_toggle = uhci_set_toggle,
  .create_eop = uhci_create_eop,
  .remove_eop = uhci_remove_eop
};

void
uhci_init (void)
{
  struct pci_dev *pd;
  int dev_num;

  dev_num = 0;
  while ((pd = pci_get_dev_by_class (PCI_MAJOR_SERIALBUS, PCI_MINOR_USB,
				     PCI_USB_IFACE_UHCI, dev_num)) != NULL)
    {
      struct pci_io *io;
      struct uhci_info *ui;
      int i;

      dev_num++;

      /* find IO space */
      io = NULL;
      while ((io = pci_io_enum (pd, io)) != NULL)
	{
	  if (pci_io_size (io) == UHCI_REGSZ)
	    break;
	}

      /* not found, next PCI */
      if (io == NULL)
	continue;

      ui = uhci_create_info (io);
      ui->dev = pd;

      uhci_detect_ports (ui);

      pci_write_config16 (ui->dev, UHCI_REG_LEGSUP, 0x8f00);
      pci_reg_write16 (ui->io, UHCI_REG_USBCMD, USB_CMD_HCRESET);
      asm volatile ("mfence":::"memory");
      timer_usleep (5);
      if (pci_reg_read16 (ui->io, UHCI_REG_USBCMD) & USB_CMD_HCRESET)
        printf ("reset failed!\n");
      pci_reg_write16 (ui->io, UHCI_REG_USBINTR, 0);
      pci_reg_write16 (ui->io, UHCI_REG_USBCMD, 0);

      pci_reg_write16 (ui->io, UHCI_REG_PORTSC1, 0);
      pci_reg_write16 (ui->io, UHCI_REG_PORTSC2, 0);

      pci_reg_write8 (ui->io, UHCI_REG_SOFMOD, 64);
      pci_reg_write32 (ui->io, UHCI_REG_FLBASEADD, vtop (ui->frame_list));
      pci_reg_write16 (ui->io, UHCI_REG_FRNUM, 0);
      asm volatile ("mfence":::"memory");

      /* deactivate SMM junk, only enable IRQ */
      pci_write_config16 (ui->dev, UHCI_REG_LEGSUP, 0x2000);

      asm volatile ("mfence":::"memory");

      pci_reg_write16 (ui->io, UHCI_REG_USBCMD,
                       USB_CMD_RS | USB_CMD_CONFIGURE | USB_CMD_MAX_PACKET);
      pci_reg_write16 (ui->io, UHCI_REG_USBINTR,
		       USB_INTR_SHORT | USB_INTR_IOC |
		       USB_INTR_TIMEOUT | USB_INTR_RESUME);

      asm volatile ("mfence":::"memory");

      printf ("UHCI: Enabling %d root ports\n", ui->num_ports);
      ui->attached_ports = 0;
      for (i = 0; i < ui->num_ports; i++)
	ui->attached_ports += uhci_enable_port (ui, i);

      pci_register_irq (pd, uhci_irq, ui);

      usb_register_host (&uhci_host, ui);
    }
}

#define UHCI_PORT_TIMEOUT	1000
static int
uhci_enable_port (struct uhci_info *ui, int idx)
{
  uint16_t status;
  int time, stable_since;
  int count;
  int port;

  port = UHCI_REG_PORTSC1 + idx * 2;

  status = 0xffff;
  stable_since = 0;
  for (time = 0; ; time += 25) 
    {
      uint16_t new_status;
      new_status = pci_reg_read16 (ui->io, port);
      if (status != (new_status & USB_PORT_CONNECTSTATUS)
          || new_status & USB_PORT_CONNECTCHG) 
        {
          if (new_status & USB_PORT_CONNECTCHG)
            pci_reg_write16 (ui->io, port, (new_status & ~0xe80a) | USB_PORT_CONNECTCHG);
          stable_since = time;
          status = new_status & USB_PORT_CONNECTSTATUS;
        }
      else if (time - stable_since >= 100)
        break;
      else if (time >= 1500)
        return 0;
      timer_msleep (25);
    }

  if (!(status & USB_PORT_CONNECTSTATUS))
    return 0;

  for (count = 0; count < 3; count++) 
    {
      status = pci_reg_read16 (ui->io, port) & ~0xe80a;
      pci_reg_write16 (ui->io, port, status | USB_PORT_RESET);
      timer_msleep (50);

      status = pci_reg_read16 (ui->io, port) & ~0xe80a;
      pci_reg_write16 (ui->io, port, status & ~USB_PORT_RESET);
      timer_usleep (10);

      status = pci_reg_read16 (ui->io, port) & ~0xe80a;
      pci_reg_write16 (ui->io, port, status & ~(USB_PORT_CONNECTCHG | USB_PORT_CHANGE));

      status = pci_reg_read16 (ui->io, port) & ~0xe80a;
      pci_reg_write16 (ui->io, port, status | USB_PORT_ENABLE);

      status = pci_reg_read16 (ui->io, port);
      if (status & USB_PORT_ENABLE)
        break;
    }

  if (!(status & USB_PORT_CONNECTSTATUS))
    {
      pci_reg_write16 (ui->io, port, 0);
      return 0;
    }

  return 1;
}


static void
dump_regs (struct uhci_info *ui)
{
  int regs[] = { 0, 2, 4, 6, 8, 0xc, 0x10, 0x12 };
  int sz[] = { 2, 2, 2, 2, 4, 2, 2, 2 };
  char *name[] =
    { "cmd", "sts", "intr", "frnum", "base", "sofmod", "portsc1", "portsc2" };
  int i;
  printf ("UHCI registers:\n");
  for (i = 0; i < 8; i++)
    {
      printf ("%s: %x\n", name[i], (sz[i] == 2) ?
	      pci_reg_read16 (ui->io, regs[i]) :
	      pci_reg_read32 (ui->io, regs[i]));

    }
}


static void
uhci_destroy_info (struct uhci_info *ui)
{
  palloc_free_page (ui->frame_list);
  palloc_free_page (ui->td_pool);
  palloc_free_page (ui->qh_pool);
  bitmap_destroy (ui->qh_used);
  bitmap_destroy (ui->td_used);
  free (ui);
}

static struct uhci_info *
uhci_create_info (struct pci_io *io)
{
  struct uhci_info *ui;
  int i;

  ui = malloc (sizeof (struct uhci_info));

  ui->io = io;
  lock_init (&ui->lock);

  /* create an empty schedule */
  ui->frame_list = palloc_get_page (PAL_ASSERT | PAL_NOCACHE);
  memset (ui->frame_list, 0, PGSIZE);
  for (i = 0; i < FRAME_LIST_ENTRIES; i++)
    ui->frame_list[i].terminate = 1;

  /* permit 3 timeouts */
  ui->timeouts = 3;
//  thread_create ("uhci watchdog", PRI_MIN,
//		 (thread_func *) uhci_stall_watchdog, ui);
  ui->td_pool = palloc_get_page (PAL_ASSERT | PAL_NOCACHE);
  ui->td_used = bitmap_create (TD_ENTRIES);
  ui->qh_pool = palloc_get_page (PAL_ASSERT | PAL_NOCACHE);
  ui->qh_used = bitmap_create (QH_ENTRIES);
  sema_init (&ui->td_sem, TD_ENTRIES);

  list_init (&ui->devices);
  list_init (&ui->waiting);

  return ui;
}


static int
uhci_tx_pkt (host_eop_info hei, int token, void *pkt, int min_sz,
	     int max_sz, int *in_sz, bool wait)
{
  struct uhci_eop_info *ue;
  struct uhci_dev_info *ud;

  ASSERT (min_sz <= max_sz);

  /* can't have page overlap */
  if (pkt != NULL)
    {
      ASSERT (max_sz > 0);
      ASSERT (pg_no (pkt + max_sz - 1) == pg_no (pkt));
    }

  ue = hei;
  ud = ue->ud;

  /* don't bother if ports are down */
  if (ud->ui->attached_ports == 0)
    {
      return USB_HOST_ERR_NODEV;
    }

  /* setup token acts to synchronize data toggle */
  if (token == USB_TOKEN_SETUP)
    ue->toggle = 0;

  if (min_sz != 0)
    {
      return uhci_tx_pkt_bulk (ue, token, pkt, max_sz, in_sz);
    }
  else
    {
      if (wait == false)
	{
	  if (in_sz != NULL)
	    *in_sz = max_sz;
	  return uhci_tx_pkt_now (ue, token, pkt, max_sz);
	}
      else
	{
	  return uhci_tx_pkt_wait (ue, token, pkt, max_sz, in_sz);
	}
    }

  return 0;
}

static int
uhci_tx_pkt_bulk (struct uhci_eop_info *ue, int token, void *buf,
		  int sz, int *tx)
{
  /* XXX this can be made to use async packets */
  int bytes = 0;
  int txed = 0;

  /* send data in max_pkt sized chunks */
  while (bytes < sz)
    {
      int to_tx, pkt_txed;
      int left;
      int err;
      bool wait_on_pkt;

      left = sz - bytes;
      to_tx = (left > ue->maxpkt) ? ue->maxpkt : left;
      wait_on_pkt = (left <= to_tx) ? true : false;

      pkt_txed = 0;
      err = uhci_tx_pkt (ue, token, buf + bytes, 0, to_tx, &pkt_txed, 
			 wait_on_pkt);
      if (err)
	{
	  if (tx != NULL)
	    *tx = txed;
	  return err;
	}
      txed += pkt_txed;
      bytes += pkt_txed;
    }

  if (tx != NULL)
    *tx = txed;

  return USB_HOST_ERR_NONE;
}

static int
token_to_pid (int token)
{
  switch (token)
    {
    case USB_TOKEN_SETUP:
      return USB_PID_SETUP;
    case USB_TOKEN_IN:
      return USB_PID_IN;
    case USB_TOKEN_OUT:
      return USB_PID_OUT;
    default:
      PANIC ("Unknown USB token\n");
    }
}

static void
uhci_setup_td (struct tx_descriptor *td, int dev_addr, int token,
	       int eop, void *pkt, int sz, int toggle, bool ls)
{
  td->buf_ptr = (sz == 0) ? 0 : vtop (pkt);

  td->token.pid = token_to_pid (token);
  td->token.dev_addr = dev_addr;
  td->token.end_point = eop;
  td->token.data_toggle = toggle;
  td->token.maxlen = sz - 1;
//  td->control.ls = ls;

  td->control.actual_len = 0;
  td->control.active = 1;
  td->flp.qh_select = 0;
  td->flp.depth_select = 0;

  /* kill packet if too many errors */
  td->control.error_limit = 3;
}

static int
uhci_tx_pkt_now (struct uhci_eop_info *ue, int token, void *pkt, int sz)
{
  struct tx_descriptor *td;
  struct uhci_dev_info *ud;

  ud = ue->ud;

  uhci_lock (ud->ui);

  td = uhci_acquire_td (ud->ui);
  memset (td, 0, sizeof (struct tx_descriptor));
  uhci_setup_td (td, ud->dev_addr, token, ue->eop, pkt, sz, ue->toggle,
		 ud->low_speed);
  td->control.ioc = 1;

  uhci_stop (ud->ui);

  uhci_add_td_to_qh (ud->qh, td);
  td->flags = TD_FL_ASYNC | TD_FL_USED;

  uhci_run (ud->ui);
  uhci_unlock (ud->ui);

  ue->toggle ^= 1;
  return USB_HOST_ERR_NONE;
}

static int
uhci_tx_pkt_wait (struct uhci_eop_info *ue, int token, void *pkt,
		  int max_sz, int *in_sz)
{
  enum intr_level old_lvl;
  struct tx_descriptor *td;
  struct usb_wait w;
  int err;
  struct uhci_dev_info *ud;
  int n;

  ud = ue->ud;

  uhci_lock (ud->ui);

  td = uhci_acquire_td (ud->ui);
  memset (td, 0, sizeof (struct tx_descriptor));

  uhci_setup_td (td, ud->dev_addr, token, ue->eop, pkt, max_sz, ue->toggle,
		 ud->low_speed);
  td->control.ioc = 1;

  w.td = td;
  w.ud = ud;
  sema_init (&w.sem, 0);

  uhci_stop (ud->ui);

  /* put into device's queue and add to waiting packet list */
  uhci_add_td_to_qh (ud->qh, td);
  td->flags = TD_FL_USED;

  list_push_back (&ud->ui->waiting, &w.peers);

  /* reactivate controller and wait */
  old_lvl = intr_disable ();
  uhci_run (ud->ui);
  uhci_unlock (ud->ui);
  sema_down (&w.sem);
  intr_set_level (old_lvl);

  if (in_sz != NULL)
    {
      if (w.td->control.actual_len == 0x7ff)
	*in_sz = 0;
      else
	*in_sz = w.td->control.actual_len + 1;
    }

  if (w.td->control.bitstuff)
    err = USB_HOST_ERR_BITSTUFF;
  else if (w.td->control.timeout)
    err = USB_HOST_ERR_TIMEOUT;
  else if (w.td->control.nak)
    err = USB_HOST_ERR_NAK;
  else if (w.td->control.babble)
    err = USB_HOST_ERR_BABBLE;
  else if (w.td->control.buffer_error)
    err = USB_HOST_ERR_BUFFER;
  else if (w.td->control.stalled)
    err = USB_HOST_ERR_STALL;
  else
    {
      err = USB_HOST_ERR_NONE;
      ue->toggle ^= 1;
    }

  uhci_release_td (ud->ui, td);

#if 0
  printf ("%s to device %d:\n",
          token == USB_TOKEN_OUT ? "OUT"
          : token == USB_TOKEN_IN ? "IN"
          : token == USB_TOKEN_SETUP ? "SETUP"
          : "unknown",
          ud->dev_addr);
  n = (w.td->control.actual_len + 1) & 0x7ff;
  hex_dump (0, pkt, n, true); 
  if (err) 
    printf ("tx_pkt_wait: err=%d\n", err);
#endif

  return err;
}

static void
uhci_add_td_to_qh (struct queue_head *qh, struct tx_descriptor *td)
{
  struct frame_list_ptr *fp;

  ASSERT (td != NULL);

  td->head = &qh->qelp;
  if (qh->qelp.terminate == 1)
    {
      /* queue is empty */
      td->flp.terminate = 1;
      barrier ();
      td->flp.flp = 0;
      qh->qelp.flp = ptr_to_flp (vtop (td));
      qh->qelp.terminate = 0;
    }
  else
    {
      /* find the last element in the queue */
      fp = ptov (flp_to_ptr (qh->qelp.flp));
      ASSERT (qh->qelp.terminate == 0);
      while (!fp->terminate)
	{
	  fp = ptov (flp_to_ptr (fp->flp));
	}

      /* set TD to terminated ptr */
      td->flp = *fp;

      fp->qh_select = 0;
      fp->depth_select = 0;
      fp->flp = ptr_to_flp (vtop (td));
      barrier ();
      fp->terminate = 0;
    }
}

static void
uhci_irq (void *uhci_data)
{
  struct uhci_info *ui;
  uint16_t status;

  ui = uhci_data;
  status = pci_reg_read16 (ui->io, UHCI_REG_USBSTS);
  if (status & USB_STATUS_PROCESSERR)
    {
      dump_all_qh (ui);
      dump_regs (ui);
      PANIC ("UHCI: Malformed schedule");
    }
  else if (status & USB_STATUS_HOSTERR)
    {
      dump_all_qh (ui);
      dump_regs (ui);
      PANIC ("UHCI: Host system error");
    }
  else if (status & USB_STATUS_INTERR)
    {
      /* errors */
      pci_reg_write16 (ui->io, UHCI_REG_USBSTS, USB_STATUS_INTERR);
    }

  if (status & USB_STATUS_USBINT)
    {
      /* turn off interrupt */
      uhci_stop_unlocked (ui);
      pci_reg_write16 (ui->io, UHCI_REG_USBSTS, USB_STATUS_USBINT);
      uhci_process_completed (ui);
      uhci_run_unlocked (ui);
    }
}

static int
uhci_process_completed (struct uhci_info *ui)
{
  struct list_elem *li;
  int completed = 0;
  size_t start = 0;

  li = list_begin (&ui->waiting);
  while (li != list_end (&ui->waiting))
    {
      struct usb_wait *uw;
      struct list_elem *next;

      next = list_next (li);
      uw = list_entry (li, struct usb_wait, peers);

      if (!uw->td->control.active)
	{
	  list_remove (li);
	  if (uw->td->control.error_limit == 0 || uw->td->control.stalled)
	    {
	      uhci_remove_error_td (uw->td);
	    }
	  uw->td->flags = 0;
	  sema_up (&uw->sem);
	  completed++;
	}
      li = next;
    }

  /* must be a completed async TD.. */
  /* is this too time consuming? I hope not */
  if (completed != 0)
    return completed;

  while (start < TD_ENTRIES)
    {
      struct tx_descriptor *td;

      start = bitmap_scan (ui->td_used, start, 1, true);
      if (start == BITMAP_ERROR)
	break;

      td = td_from_pool (ui, start);

      if (!td->control.active && (td->flags & TD_FL_ASYNC) &&
	  (td->flags & TD_FL_USED))
	{
	  if (td->control.error_limit == 0 || td->control.stalled)
	    {
	      uhci_remove_error_td (td);
	    }
	  uhci_release_td (ui, td);
	  completed++;
	}
      start++;
    }

  return completed;
}

static void
uhci_remove_error_td (struct tx_descriptor *td)
{
  struct frame_list_ptr *fp;
  uint32_t td_flp;

  ASSERT (td->head != NULL);

  fp = td->head;
  td_flp = ptr_to_flp (vtop (td));
  while (fp->flp != td_flp)
    {
      ASSERT (fp->terminate == 0);
      fp = ptov (flp_to_ptr (fp->flp));
    }
  *fp = td->flp;
}

static int
uhci_detect_change (host_info hi)
{
  struct uhci_info *ui;
  int change;
  int i;

  ui = hi;
  change = 0;
  uhci_lock (ui);
  for (i = 0; i < ui->num_ports; i++)
    {
      change = check_and_flip_change (ui, i);
      if (change != 0)
	break;
    }
  uhci_unlock (ui);

  return change;
}

static int
check_and_flip_change (struct uhci_info *ui, int port)
{
  int val;
  int reg;

  reg = UHCI_REG_PORTSC1 + port * 2;
  val = pci_reg_read16 (ui->io, reg);
  if (val & USB_PORT_CHANGE)
    {
      pci_reg_write16 (ui->io, reg, val & ~(USB_PORT_CHANGE));
      return 1;
    }

  return 0;
}

static host_dev_info
uhci_create_chan (host_info hi, int dev_addr, int ver)
{
  struct uhci_info *ui;
  struct uhci_dev_info *ud;
  int i;

  ASSERT (dev_addr <= 127 && dev_addr >= 0);

  ui = hi;

  ud = malloc (sizeof (struct uhci_dev_info));
  ud->dev_addr = dev_addr;
  ud->low_speed = (ver == USB_VERSION_1_0) ? true : false;

  ud->errors = 0;
  ud->ui = ui;
  lock_init (&ud->lock);

  uhci_lock (ui);

  ud->qh = qh_alloc (ud->ui);

  /* queue data */
  memset (ud->qh, 0, sizeof (*ud->qh));
  ud->qh->qelp.terminate = 1;
  barrier ();
  ud->qh->qelp.flp = 0;
  ud->qh->qelp.qh_select = 0;
  ud->qh->qhlp.qh_select = 1;

  uhci_stop (ui);

  /* add to queues in frame list */
  ud->qh->qhlp.flp = ui->frame_list[0].flp;
  ud->qh->qhlp.terminate = ui->frame_list[0].terminate;
  for (i = 0; i < FRAME_LIST_ENTRIES; i++)
    {
      ui->frame_list[i].flp = ptr_to_flp (vtop (ud->qh));
      ui->frame_list[i].qh_select = 1;
      ui->frame_list[i].terminate = 0;
    }

  /* add to device list */
  list_push_back (&ui->devices, &ud->peers);

  uhci_run (ui);
  uhci_unlock (ui);

  return ud;
}

static void
uhci_destroy_chan (host_dev_info hd)
{
  struct uhci_dev_info *ud;
  struct list_elem *li;

  ud = hd;
  uhci_lock (ud->ui);

  uhci_stop (ud->ui);

  uhci_remove_qh (ud->ui, ud->qh);

  /* wake up all waiting */
  li = list_begin (&ud->ui->waiting);
  while (li != list_end (&ud->ui->waiting))
    {
      struct usb_wait *w;
      w = list_entry (li, struct usb_wait, peers);
      if (w->ud == ud)
	{
	  sema_up (&w->sem);
	  list_remove (li);
	}
      li = list_next (li);
    }

  list_remove (&ud->peers);

  uhci_run (ud->ui);

  qh_free (ud->ui, ud->qh);

  uhci_unlock (ud->ui);

  free (ud);
}

/**
 * Remove a queue from the UHCI schedule
 */
static void
uhci_remove_qh (struct uhci_info *ui, struct queue_head *qh)
{
  uintptr_t qh_flp;

  ASSERT (lock_held_by_current_thread (&ui->lock));
  ASSERT (uhci_is_stopped (ui));
  ASSERT (qh != NULL);

  qh_flp = ptr_to_flp (vtop (qh));
  /* remove from host queue */
  if (ui->frame_list[0].flp == qh_flp)
    {
      int i;
      /* up top */
      for (i = 0; i < FRAME_LIST_ENTRIES; i++)
	{
	  ui->frame_list[i] = qh->qhlp;
	}
    }
  else
    {
      /* in the middle */
      struct frame_list_ptr *fp;
      struct frame_list_ptr *prev;

      fp = ptov (flp_to_ptr (ui->frame_list[0].flp));
      ASSERT (!fp->terminate);
      do
	{
	  prev = fp;
	  fp = ptov (flp_to_ptr (fp->flp));
	}
      while (!fp->terminate && fp->flp != qh_flp);
      *prev = qh->qhlp;
    }
}

/**
 * Put UHCI into stop state
 * Wait until status register reflects setting
 */
static void
uhci_stop (struct uhci_info *ui)
{
  ASSERT (intr_get_level () != INTR_OFF);
  ASSERT (lock_held_by_current_thread (&ui->lock));

  uhci_stop_unlocked (ui);
}

static void
uhci_stop_unlocked (struct uhci_info *ui)
{
  uint16_t cmd;
  int i;

  cmd = pci_reg_read16 (ui->io, UHCI_REG_USBCMD);
  cmd = cmd & ~USB_CMD_RS;

  pci_reg_write16 (ui->io, UHCI_REG_USBCMD, cmd);

  /* wait for execution schedule to finish up */
  for (i = 0; i < 1000; i++)
    {
      if (uhci_is_stopped (ui))
	return;
    }

  PANIC ("UHCI: Controller did not halt\n");

}

static void
uhci_run_unlocked (struct uhci_info *ui)
{
  uint16_t cmd;
  cmd = pci_reg_read16 (ui->io, UHCI_REG_USBCMD);
  cmd = cmd | USB_CMD_RS | USB_CMD_MAX_PACKET;
  pci_reg_write16 (ui->io, UHCI_REG_USBCMD, cmd);
}

/**
 * Put UHCI into 'Run' State 
 */
static void
uhci_run (struct uhci_info *ui)
{
  ASSERT (lock_held_by_current_thread (&ui->lock));
  uhci_run_unlocked (ui);
}

static struct tx_descriptor *
uhci_acquire_td (struct uhci_info *ui)
{
  size_t td_idx;
  struct tx_descriptor *td;

  ASSERT (lock_held_by_current_thread (&ui->lock));
  ASSERT (!uhci_is_stopped (ui));

  sema_down (&ui->td_sem);
  td_idx = bitmap_scan_and_flip (ui->td_used, 0, 1, false);
  ASSERT (td_idx != BITMAP_ERROR);
  td = td_from_pool (ui, td_idx);

  return td;
}

static void
uhci_modify_chan (host_dev_info hd, int dev_addr, int ver)
{
  struct uhci_dev_info *ud;

  ud = hd;
  ud->dev_addr = dev_addr;
  ud->low_speed = (ver == USB_VERSION_1_0) ? true : false;
}

static void
uhci_set_toggle (host_eop_info he, int toggle) 
{
  struct uhci_eop_info *ue;

  ue = he;
  ue->toggle = toggle;
}

static void
dump_all_qh (struct uhci_info *ui)
{
  struct list_elem *li;

  printf ("schedule: %x...", vtop (ui->frame_list));
  printf ("%x", *((uint32_t *) ui->frame_list));
  li = list_begin (&ui->devices);
  while (li != list_end (&ui->devices))
    {
      struct uhci_dev_info *ud;
      ud = list_entry (li, struct uhci_dev_info, peers);
      dump_qh (ud->qh);
      li = list_next (li);
    }
}

static void
dump_qh (struct queue_head *qh)
{
  struct frame_list_ptr *fp;
  printf ("qh: %p %x\n", qh, vtop (qh));
  fp = &qh->qelp;
  while (!fp->terminate)
    {
      printf ("%x %x\n", *((uint32_t *) fp), *(uint32_t *) (fp + 1));
      fp = ptov (flp_to_ptr (fp->flp));
    }
  printf ("%x %x\n\n", *(uint32_t *) fp, *(uint32_t *) (fp + 1));
}

static struct tx_descriptor *
td_from_pool (struct uhci_info *ui, int idx)
{
  ASSERT (idx >= 0 && idx < TD_ENTRIES);
  return (((void *) ui->td_pool) + idx * 32);
}

static void
uhci_detect_ports (struct uhci_info *ui)
{
  int i;
  ui->num_ports = 0;
  for (i = 0; i < UHCI_MAX_PORTS; i++)
    {
      uint16_t status;
      status = pci_reg_read16 (ui->io, UHCI_REG_PORTSC1 + i * 2);
      if (!(status & 0x0080) || status == 0xffff)
	return;
      ui->num_ports++;
    }
}

static void
uhci_stall_watchdog (struct uhci_info *ui)
{
  while (1)
    {
      int rmved;
      timer_msleep (1000);
      printf ("watchdog\n");
      uhci_lock (ui);
      uhci_stop (ui);
      rmved = uhci_remove_stalled (ui);
      if (rmved > 0)
	printf ("removed stalled packet in watchdog\n");
      uhci_run (ui);
      uhci_unlock (ui);
    }
}

static int
uhci_remove_stalled (struct uhci_info *ui)
{
  struct list_elem *li;
  int rmved;

  rmved = 0;
  li = list_begin (&ui->waiting);

  intr_disable ();

  while (li != list_end (&ui->waiting))
    {
      struct usb_wait *uw;
      struct list_elem *next;
      uint32_t ctrl;

      next = list_next (li);
      uw = list_entry (li, struct usb_wait, peers);

      if ((!uw->td->control.active && uw->td->control.stalled) ||
	  (uw->td->control.nak))
	{
	  memcpy (&ctrl, &uw->td->control, 4);
	  printf ("CTRL: %x\n", ctrl);
	  list_remove (li);
	  uhci_remove_error_td (uw->td);
	  sema_up (&uw->sem);
	  rmved++;
	}
      li = next;
    }

  intr_enable ();

  return rmved;
}

static void
uhci_release_td (struct uhci_info *ui, struct tx_descriptor *td)
{
  int ofs = (uintptr_t) td - (uintptr_t) ui->td_pool;
  int entry = ofs / 32;

  ASSERT (entry < TD_ENTRIES);

  td->flags = 0;
  bitmap_reset (ui->td_used, entry);
  sema_up (&ui->td_sem);
}

static host_eop_info
uhci_create_eop (host_dev_info hd, int eop, int maxpkt)
{
  struct uhci_dev_info *ud;
  struct uhci_eop_info *e;

  ud = hd;

  e = malloc (sizeof (struct uhci_eop_info));
  e->eop = eop;
  e->ud = ud;
  e->maxpkt = maxpkt;
  e->toggle = 0;

  return e;
}

static void
uhci_remove_eop (host_eop_info hei)
{
  free (hei);
}

static struct queue_head *
qh_alloc (struct uhci_info *ui)
{
  size_t qh_idx;
  struct queue_head *qh;

  ASSERT (lock_held_by_current_thread (&ui->lock));

  qh_idx = bitmap_scan_and_flip (ui->qh_used, 0, 1, false);
  if (qh_idx == BITMAP_ERROR)
    {
      PANIC ("UHCI: Too many queue heads in use-- runaway USB stack?\n");
    }
  qh = (void *) (((intptr_t) ui->qh_pool) + qh_idx * 16);

  return qh;
}

static void
qh_free (struct uhci_info *ui, struct queue_head *qh)
{
  size_t entry;
  ASSERT (lock_held_by_current_thread (&ui->lock));

  entry = ((intptr_t) qh - (intptr_t) ui->qh_pool) / 16;
  bitmap_reset (ui->qh_used, entry);
}
