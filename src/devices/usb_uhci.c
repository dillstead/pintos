/**
 * Universal Host Controller Interface driver
 * TODO:
 *	Stall timeouts
 * 	Better (any) root hub handling
 */

#include <round.h>
#include <stdio.h>
#include <string.h>
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

#define UHCI_LP_POINTER         0xfffffff0 /* Pointer field. */
#define UHCI_LP_TERMINATE       0x00000001 /* 1=pointer not valid */
#define UHCI_LP_QUEUE_HEAD      0x00000002 /* 1=points to queue head,
                                              0=points to tx descriptor. */
#define UHCI_LP_DEPTH           0x00000004 /* Tx descriptor only:
                                              1=execute depth first,
                                              0=execute breadth first. */
#pragma pack(1)
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
  uint32_t next_td;
  struct td_control control;
  struct td_token token;
  uint32_t buf_ptr;

  uint32_t flags;
  struct list_elem td_elem;     /* Element in queue_head's td_list. */
  size_t offset;
};

struct queue_head
{
  uint32_t next_qh;             /* Queue head link pointer. */
  uint32_t first_td;            /* Queue element link pointer. */

  struct list_elem qh_elem;     /* Element in uhci_info's qh_list. */
  struct list td_list;          /* List of tx_descriptor structures. */
  struct semaphore completion;  /* Upped by interrupt on completion. */
};
#pragma pack()

struct uhci_info
{
  struct pci_dev *dev;
  struct pci_io *io;		/* pci io space */
  struct lock lock;
  uint32_t *frame_list;         /* Page-aligned list of 1024 frame pointers. */
  struct list qh_list;          /* List of queue_head structuress. */
  struct queue_head *term_qh;

  uint8_t num_ports;
  uint8_t attached_ports;
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

static int uhci_dev_control (host_eop_info, struct usb_setup_pkt *,
                             void *data, size_t *size);
static int uhci_dev_bulk (host_eop_info, bool out, void *data, size_t *size);
static int uhci_detect_change (host_info);


static struct queue_head *allocate_queue_head (void);
static struct tx_descriptor *allocate_transfer_descriptor (struct queue_head *);


static void uhci_process_completed (struct uhci_info *ui);



static struct uhci_info *uhci_create_info (struct pci_io *io);
static void uhci_destroy_info (struct uhci_info *ui);
static host_eop_info uhci_create_eop (host_dev_info hd, int eop, int maxpkt);
static void uhci_remove_eop (host_eop_info hei);
static host_dev_info uhci_create_chan (host_info hi, int dev_addr, int ver);
static void uhci_destroy_chan (host_dev_info);
static void uhci_modify_chan (host_dev_info, int dev_addr, int ver);

static int check_and_flip_change (struct uhci_info *ui, int reg);
#define uhci_is_stopped(x) 	(pci_reg_read16((x)->io, UHCI_REG_USBSTS) \
				& USB_STATUS_HALTED)
#define uhci_port_enabled(x, y) (pci_reg_read16((x)->io, (y)) & USB_PORT_ENABLE)
static void uhci_setup_td (struct tx_descriptor *td, int dev_addr, int token,
			   int eop, void *pkt, int sz, int toggle, bool ls);
static int uhci_enable_port (struct uhci_info *ui, int port);
static void uhci_irq (void *uhci_data);
static void uhci_detect_ports (struct uhci_info *ui);

static void dump_regs (struct uhci_info *ui);

void uhci_init (void);


static struct usb_host uhci_host = {
  .name = "UHCI",
  .dev_control = uhci_dev_control,
  .dev_bulk = uhci_dev_bulk,
  .detect_change = uhci_detect_change,
  .create_dev_channel = uhci_create_chan,
  .remove_dev_channel = uhci_destroy_chan,
  .modify_dev_channel = uhci_modify_chan,
  .create_eop = uhci_create_eop,
  .remove_eop = uhci_remove_eop
};

static uint32_t
make_lp (void *p) 
{
  uint32_t q = vtop (p);
  ASSERT ((q & UHCI_LP_POINTER) == q);
  return q;
}

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

      //dump_regs (ui);

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
      pci_reg_write32 (ui->io, UHCI_REG_FLBASEADD, make_lp (ui->frame_list));
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
  //printf ("time=%d stable_since=%d\n", time, stable_since);

  if (!(status & USB_PORT_CONNECTSTATUS))
    return 0;

  for (count = 0; count < 3; count++) 
    {
      status = pci_reg_read16 (ui->io, port) & ~0xe80a;
      //printf ("read %x, write %x, ", status, status | USB_PORT_RESET);
      pci_reg_write16 (ui->io, port, status | USB_PORT_RESET);
      timer_msleep (50);

      status = pci_reg_read16 (ui->io, port) & ~0xe80a;
      //printf ("read %x, write %x, ", status, status & ~USB_PORT_RESET);
      pci_reg_write16 (ui->io, port, status & ~USB_PORT_RESET);
      timer_usleep (10);

      status = pci_reg_read16 (ui->io, port) & ~0xe80a;
      //printf ("read %x, write %x, ", status, status & ~(USB_PORT_CONNECTCHG | USB_PORT_CHANGE));
      pci_reg_write16 (ui->io, port, status & ~(USB_PORT_CONNECTCHG | USB_PORT_CHANGE));

      status = pci_reg_read16 (ui->io, port) & ~0xe80a;
      //printf ("read %x, write %x, ", status, status | USB_PORT_ENABLE);
      pci_reg_write16 (ui->io, port, status | USB_PORT_ENABLE);

      status = pci_reg_read16 (ui->io, port);
      //printf ("read %x\n", status);
      if (status & USB_PORT_ENABLE)
        break;
    }

  if (!(status & USB_PORT_CONNECTSTATUS))
    {
      //printf ("port %d not connected\n", idx);
      pci_reg_write16 (ui->io, port, 0);
      return 0;
    }

  //dump_regs (ui);
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
  printf ("UHCI registers: ");
  for (i = 0; i < 8; i++)
    {
      if (i)
        printf (", ");
      printf ("%s: %x", name[i], (sz[i] == 2) ?
	      pci_reg_read16 (ui->io, regs[i]) :
	      pci_reg_read32 (ui->io, regs[i]));
    }
  printf (", legsup=%x\n", pci_read_config16 (ui->dev, UHCI_REG_LEGSUP));
}


static void
uhci_destroy_info (struct uhci_info *ui)
{
  palloc_free_page (ui->frame_list);
  free (ui);
}

static struct uhci_info *
uhci_create_info (struct pci_io *io)
{
  struct uhci_info *ui;
  struct queue_head *qh1, *qh2;
  struct tx_descriptor *td;
  int i;

  ui = malloc (sizeof (struct uhci_info));

  ui->io = io;
  lock_init (&ui->lock);

  /* Create initial queue head and put on qh list. */
  list_init (&ui->qh_list);

  qh1 = allocate_queue_head ();
  list_push_back (&ui->qh_list, &qh1->qh_elem);

  ui->term_qh = qh2 = allocate_queue_head ();
  qh1->next_qh = make_lp (qh2) | UHCI_LP_QUEUE_HEAD;

  td = allocate_transfer_descriptor (qh2);
  td->token.maxlen = 0x7ff;
  td->token.dev_addr = 0x7f;
  td->token.pid = USB_TOKEN_IN;

  /* Create an empty schedule */
  ui->frame_list = palloc_get_page (PAL_ASSERT | PAL_NOCACHE);
  for (i = 0; i < FRAME_LIST_ENTRIES; i++)
    ui->frame_list[i] = make_lp (qh1) | UHCI_LP_QUEUE_HEAD;
  //printf ("frame_list=%p: %08x\n", ui->frame_list, ui->frame_list[0]);

//  thread_create ("uhci watchdog", PRI_MIN,
//		 (thread_func *) uhci_stall_watchdog, ui);

  return ui;
}

static void
print_td_list (struct queue_head *qh) 
{
  struct list_elem *e;
  
  for (e = list_begin (&qh->td_list); e != list_end (&qh->td_list);
       e = list_next (e)) 
    {
      struct tx_descriptor *td = list_entry (e, struct tx_descriptor, td_elem);
      printf ("  %08x: next_td=%08x control=%08x %d,%d,%d,%d,%x buf_ptr=%08x\n",
              vtop (td), td->next_td, *(uint32_t *) &td->control,
              (td->token.maxlen + 1) & 0x7ff, td->token.data_toggle,
              td->token.end_point, td->token.dev_addr, td->token.pid,
              td->buf_ptr);
      hex_dump (0, ptov (td->buf_ptr), (td->token.maxlen + 1) & 0x7ff, true);
    }
}

static void
print_qh_list (struct uhci_info *ui) 
{
  struct list_elem *e;
  
  for (e = list_begin (&ui->qh_list); e != list_end (&ui->qh_list);
       e = list_next (e)) 
    {
      struct queue_head *qh = list_entry (e, struct queue_head, qh_elem);
      printf ("%08x: next_qh=%08x first_td=%08x\n",
              vtop (qh), qh->next_qh, qh->first_td);
      print_td_list (qh);
    }
}

static struct tx_descriptor *
execute_qh (struct uhci_info *ui, struct queue_head *qh) 
{
  struct queue_head *prev_qh;
  struct tx_descriptor *last_td;
  enum intr_level old_level;

  /* Mark the final tx descriptor to interrupt us when it's
     done. */
  last_td = list_entry (list_back (&qh->td_list), struct tx_descriptor, td_elem);
  last_td->control.ioc = 1;
  
  qh->next_qh = make_lp (ui->term_qh) | UHCI_LP_QUEUE_HEAD;

  /* Add the queue head to the list of queue heads.
     qh_list is accessed from uhci_irq so we need to disable
     interrupts. */
  old_level = intr_disable ();
  list_push_back (&ui->qh_list, &qh->qh_elem);
  prev_qh = list_entry (list_prev (&qh->qh_elem), struct queue_head, qh_elem);
  prev_qh->next_qh = make_lp (qh) | UHCI_LP_QUEUE_HEAD;
  print_qh_list (ui);
  //dump_regs (ui);
  intr_set_level (old_level);

  /* Wait until the queue has been processed. */
  //printf ("down...");
  sema_down (&qh->completion);
  //printf ("up\n");
  

  /* Return the descriptor that failed, or a null pointer on
     success. */
  return (qh->first_td & UHCI_LP_TERMINATE
          ? NULL
          : ptov (qh->first_td & UHCI_LP_POINTER));
}

static int
uhci_dev_control (host_eop_info hei, struct usb_setup_pkt *setup,
                  void *data, size_t *size) 
{
  struct uhci_eop_info *ue;
  struct uhci_dev_info *ud;
  struct queue_head *qh;
  struct tx_descriptor *td;
  size_t size_alloced;
  int status_token;
  int err;

  ue = hei;
  ud = ue->ud;

  /* don't bother if ports are down */
  if (ud->ui->attached_ports == 0)
    return USB_HOST_ERR_NODEV;

  qh = allocate_queue_head ();
  if (qh == NULL)
    return USB_HOST_ERR_NOMEM;

  /* Setup stage: SETUP packet. */
  td = allocate_transfer_descriptor (qh);
  if (td == NULL)
    return USB_HOST_ERR_NOMEM;
  ue->toggle = 0;
  uhci_setup_td (td, ud->dev_addr, USB_TOKEN_SETUP, ue->eop,
                 setup, sizeof *setup, ue->toggle, ud->low_speed);

  /* Data stage: IN/OUT packets. */
  if (data != NULL) 
    {
      size_alloced = 0;
      while (size_alloced < *size) 
        {
          int packet = *size - size_alloced;
          if (packet > ue->maxpkt)
            packet = ue->maxpkt;

          td = allocate_transfer_descriptor (qh);
          if (td == NULL)
            return USB_HOST_ERR_NOMEM;

          ue->toggle = !ue->toggle;
          uhci_setup_td (td, ud->dev_addr,
                         setup->direction ? USB_TOKEN_IN : USB_TOKEN_OUT,
                         ue->eop, data + size_alloced, packet,
                         ue->toggle, ud->low_speed);
          td->control.spd = 1;
          td->offset = size_alloced;

          size_alloced += packet;
        }
    }
  
  /* Status stage: OUT/IN packet. */
  td = allocate_transfer_descriptor (qh);
  if (td == NULL)
    return USB_HOST_ERR_NOMEM;
  ue->toggle = 1;
  status_token = setup->direction ? USB_TOKEN_OUT : USB_TOKEN_IN;
  uhci_setup_td (td, ud->dev_addr, status_token, ue->eop,
                 NULL, 0, ue->toggle, ud->low_speed);

  /* Execute. */
  td = execute_qh (ud->ui, qh);
  if (td != NULL)
    ue->toggle = td->token.data_toggle;
  //printf ("execution done, td=%p\n", td);
  if (td != NULL
      && setup->direction
      && list_front (&qh->td_list) != &td->td_elem
      && list_back (&qh->td_list) != &td->td_elem
      && !td->control.active
      && !td->control.stalled
      && !td->control.buffer_error
      && !td->control.babble
      && !td->control.nak
      && !td->control.timeout
      && !td->control.bitstuff
      && (((td->control.actual_len + 1) & 0x7ff)
          < ((td->token.maxlen + 1) & 0x7ff))) 
    {
      /* Short packet detected in stream, which indicates that
         the input data is shorter than the maximum that we
         expected.

         Re-queue the Status stage. */

      *size = td->offset + ((td->control.actual_len + 1) & 0x7ff);

      //delayed_free_qh (qh);

      qh = allocate_queue_head ();
      if (qh == NULL)
        return USB_HOST_ERR_NOMEM;

      td = allocate_transfer_descriptor (qh);
      if (td == NULL)
        return USB_HOST_ERR_NOMEM;
      ue->toggle = 1;
      uhci_setup_td (td, ud->dev_addr, status_token, ue->eop,
                     NULL, 0, ue->toggle, ud->low_speed);

      //printf ("short packet, requeuing status\n");
      td = execute_qh (ud->ui, qh);
      if (td != NULL)
        ue->toggle = td->token.data_toggle;
    }
      
  if (td != NULL) 
    {
      if (td->control.bitstuff)
        err = USB_HOST_ERR_BITSTUFF;
      else if (td->control.timeout)
        err = USB_HOST_ERR_TIMEOUT;
      else if (td->control.nak)
        err = USB_HOST_ERR_NAK;
      else if (td->control.babble)
        err = USB_HOST_ERR_BABBLE;
      else if (td->control.buffer_error)
        err = USB_HOST_ERR_BUFFER;
      else if (td->control.stalled)
        err = USB_HOST_ERR_STALL;
      else
        PANIC ("unknown USB error");
    }
  else
    err = USB_HOST_ERR_NONE;

  //delayed_free_qh (qh);
  
  //printf ("err=%d\n", err);
  if (err == 0 && data != NULL)
    hex_dump (0, data, *size, true);
  return err;
}

static int
uhci_dev_bulk (host_eop_info hei, bool out, void *data, size_t *size) 
{
  struct uhci_eop_info *ue;
  struct uhci_dev_info *ud;
  struct queue_head *qh;
  struct tx_descriptor *td;
  size_t size_alloced;
  int err;

  ue = hei;
  ud = ue->ud;

  /* don't bother if ports are down */
  if (ud->ui->attached_ports == 0)
    return USB_HOST_ERR_NODEV;

  if (!out)
    memset (data, 0xcc, *size);

  qh = allocate_queue_head ();
  if (qh == NULL)
    return USB_HOST_ERR_NOMEM;

  /* Data stage: IN/OUT packets. */
  size_alloced = 0;
  while (size_alloced < *size)
    {
      int packet = *size - size_alloced;
      if (packet > ue->maxpkt)
        packet = ue->maxpkt;

      td = allocate_transfer_descriptor (qh);
      if (td == NULL)
        return USB_HOST_ERR_NOMEM;

      uhci_setup_td (td, ud->dev_addr,
                     out ? USB_TOKEN_OUT : USB_TOKEN_IN,
                     ue->eop, data + size_alloced, packet,
                     ue->toggle, ud->low_speed);
      ue->toggle = !ue->toggle;
      td->control.spd = 1;
      td->offset = size_alloced;

      size_alloced += packet;
    }
  
  /* Execute. */
  td = execute_qh (ud->ui, qh);
  if (td != NULL) 
    {
      ue->toggle = !td->token.data_toggle;
      if (!out) 
        *size = td->offset + ((td->control.actual_len + 1) & 0x7ff);
      
      if (td->control.bitstuff)
        err = USB_HOST_ERR_BITSTUFF;
      else if (td->control.timeout)
        err = USB_HOST_ERR_TIMEOUT;
      else if (td->control.nak)
        err = USB_HOST_ERR_NAK;
      else if (td->control.babble)
        err = USB_HOST_ERR_BABBLE;
      else if (td->control.buffer_error)
        err = USB_HOST_ERR_BUFFER;
      else if (td->control.stalled)
        err = USB_HOST_ERR_STALL;
      else if (!out)
        {
          /* Just a short packet. */
          printf ("short packet\n");
          err = USB_HOST_ERR_NONE;
        }
      else
        PANIC ("unknown USB error");
    }
  else
    err = USB_HOST_ERR_NONE;

  //delayed_free_qh (qh);

  if (err)
    PANIC ("err=%d\n", err);
  if (err == 0 && data != NULL && !out)
    hex_dump (0, data, *size, true);
  return err;
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
	       int eop, void *pkt, int sz, int toggle, bool ls UNUSED)
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
  td->next_td = UHCI_LP_TERMINATE;

  /* kill packet if too many errors */
  td->control.error_limit = 3;
}

static void
uhci_irq (void *uhci_data)
{
  struct uhci_info *ui;
  uint16_t status;

  //printf ("uhci_irq\n");
  ui = uhci_data;
  status = pci_reg_read16 (ui->io, UHCI_REG_USBSTS);
  if (status & USB_STATUS_PROCESSERR)
    {
      //dump_all_qh (ui);
      dump_regs (ui);
      PANIC ("UHCI: Malformed schedule");
    }
  else if (status & USB_STATUS_HOSTERR)
    {
      //dump_all_qh (ui);
      dump_regs (ui);
      PANIC ("UHCI: Host system error");
    }
  else if (status & (USB_STATUS_INTERR | USB_STATUS_USBINT))
    {
      /* errors */
      //pci_reg_write16 (ui->io, UHCI_REG_USBINTR, 0);
      pci_reg_write16 (ui->io, UHCI_REG_USBSTS,
                       status & (USB_STATUS_INTERR | USB_STATUS_USBINT));
      if (status & USB_STATUS_INTERR) 
        {
          printf ("USB_STATUS_INTERR\n");
          print_qh_list (ui);
          dump_regs (ui); 
        }
      //if (status & USB_STATUS_USBINT)
      //printf ("USB_STATUS_USBINT\n");
      barrier ();
      uhci_process_completed (ui);
    }
  else
    printf ("nothing?\n");
}

/* Queue processing finished.
   Remove from lists, wake up waiter. */
static void
finish_qh (struct queue_head *qh)
{
  struct list_elem *p = list_prev (&qh->qh_elem);
  struct queue_head *prev_qh = list_entry (p, struct queue_head, qh_elem);

  //printf ("finish_qh\n");
  prev_qh->next_qh = qh->next_qh;
  list_remove (&qh->qh_elem);
  sema_up (&qh->completion);
}

static void
uhci_process_completed (struct uhci_info *ui)
{
  struct list_elem *e;

  if (list_empty (&ui->qh_list))
    return;

  /* Note that we skip the first element in the list, which is an
     empty queue head pointed to by every frame list pointer. */
  for (e = list_next (list_begin (&ui->qh_list));
       e != list_end (&ui->qh_list); ) 
    {
      struct queue_head *qh = list_entry (e, struct queue_head, qh_elem);
      uint32_t first_td = qh->first_td;
      bool delete = false;

      barrier ();
      //printf ("first_td=%08x\n", first_td);
      if ((first_td & UHCI_LP_POINTER) == 0)
        delete = true;
      else 
        {
          struct tx_descriptor *td = ptov (first_td & UHCI_LP_POINTER);
          if (!td->control.active) 
            delete = true;
        }

      e = list_next (e);
      if (delete)
        finish_qh (qh);
    }
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

static struct queue_head *
allocate_queue_head (void) 
{
  size_t qh_size = sizeof *allocate_queue_head ();
  struct queue_head *qh = calloc (1, qh_size > 16 ? qh_size : 16);
  if (qh != NULL) 
    {
      qh->next_qh = UHCI_LP_TERMINATE;
      qh->first_td = UHCI_LP_TERMINATE;
      list_init (&qh->td_list);
      sema_init (&qh->completion, 0); 
    }
  return qh;
}

static struct tx_descriptor *
allocate_transfer_descriptor (struct queue_head *qh) 
{
  struct tx_descriptor *td = calloc (1, sizeof *td);
  if (td == NULL) 
    {
      /* FIXME: free the whole queue head. */
      return NULL; 
    }

  if (list_empty (&qh->td_list))
    qh->first_td = make_lp (td);
  else
    {
      struct list_elem *p = list_back (&qh->td_list);
      struct tx_descriptor *prev_td = list_entry (p, struct tx_descriptor,
                                                  td_elem);
      prev_td->next_td = make_lp (td);
    }
  list_push_back (&qh->td_list, &td->td_elem);
  td->next_td = UHCI_LP_TERMINATE;
  return td;
}

static host_dev_info
uhci_create_chan (host_info hi, int dev_addr, int ver)
{
  struct uhci_info *ui;
  struct uhci_dev_info *ud;

  ASSERT (dev_addr <= 127 && dev_addr >= 0);

  ui = hi;

  ud = malloc (sizeof (struct uhci_dev_info));
  ud->dev_addr = dev_addr;
  ud->low_speed = (ver == USB_VERSION_1_0) ? true : false;

  ud->errors = 0;
  ud->ui = ui;
  lock_init (&ud->lock);

  return ud;
}

static void
uhci_destroy_chan (host_dev_info hd UNUSED)
{
  /* FIXME */
  /* FIXME: wake up all waiting queue heads. */
  /* FIXME: arrange for qhs to be freed. */
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
