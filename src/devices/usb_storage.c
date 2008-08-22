/**
 * USB mass storage driver - just like the one Elvis used!
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <kernel/list.h>
#include <endian.h>
#include <debug.h>
#include "threads/malloc.h"
#include "threads/palloc.h"
#include "threads/thread.h"
#include "devices/timer.h"
#include "threads/synch.h"
#include "threads/pte.h"
#include "devices/usb.h"
#include "devices/block.h"
#include "devices/partition.h"

//http://www.usb.org/developers/defined_class
#define USB_CLASS_MASS_STORAGE	0x08

#define USB_SUBCLASS_RBC	0x01	/* reduced block commands */
#define USB_SUBCLASS_ATAPI	0x02	/* ATAPI - CD's */
#define USB_SUBCLASS_TAPE	0x03	
#define USB_SUBCLASS_UFI	0x04	/* floppy disk */
#define USB_SUBCLASS_SFF8070	0x05	
#define USB_SUBCLASS_SCSI	0x06	/* scsi transparent command set */

#define USB_PROTO_COMPLETE	0x00
#define USB_PROTO_NO_COMPLETE	0x01
#define USB_PROTO_BULK		0x50

#pragma pack(1)

#define CBW_SIG_MAGIC	0x43425355

#define CBW_FL_IN		(1 << 7)
#define CBW_FL_OUT		0

/** usbmassbulk_10.pdf - pg 13 */
/* command block wrapper */
struct msc_cbw
{
  uint32_t sig;
  uint32_t tag;
  uint32_t tx_len;
  uint8_t flags;
  uint8_t lun;
  uint8_t cb_len;		/* command block length */
  uint8_t cb[16];		/* command block */
};

#define CSW_SIG_MAGIC		0x53425355
#define CSW_STATUS_PASSED	0
#define CSW_STATUS_FAILED	1
#define CSW_STATUS_PHASE	2	/* phase error */
/* command status wrapper */
struct msc_csw
{
  uint32_t sig;
  uint32_t tag;
  uint32_t residue;
  uint8_t status;
};

struct scsi_cdb6
{
  uint8_t op;
  uint8_t lba[3];
  uint8_t len;
  uint8_t control;
};

struct scsi_cdb10
{
  uint8_t op;
  uint8_t action;
  uint32_t lba;
  uint8_t resv;
  uint16_t len;
  uint8_t control;
};

struct scsi_cdb12
{
  uint8_t op;
  uint8_t action;
  uint32_t lba;
  uint32_t len;
  uint8_t resv;
  uint8_t control;
};

struct scsi_cdb16
{
  uint8_t op;
  uint8_t action;
  uint32_t lba;
  uint32_t data;
  uint32_t len;
  uint8_t resv;
  uint8_t control;
};

struct scsi_capacity10
{
  uint32_t blocks;
  uint32_t block_len;
};

#define SCSI_OP_WRITE6			0xa
#define SCSI_OP_READ6			0x8
#define SCSI_OP_TEST_READY		0x00
#define SCSI_OP_SEEK8			0x0b
#define SCSI_OP_MODE_SENSE8		0x1a
#define SCSI_OP_MODE_SELECT8		0x15
#define SCSI_OP_READ_CAPACITY10		0x25
#define SCSI_OP_READ_CAPACITY16		0x9e
#define SCSI_OP_WRITE10			0x2a
#define SCSI_OP_READ10			0x28
#define SCSI_OP_SEEK10			0x2b
#define SCSI_OP_MODE_SENSE10		0x5a
#define SCSI_OP_MODE_SELECT10		0x55


#pragma pack()

static void *msc_attached (struct usb_iface *);
static void msc_detached (class_info);

static void msc_reset_endpoint (struct usb_endpoint *eop);

static struct usb_class storage_class = {
  .attached = msc_attached,
  .detached = msc_detached,
  .name = "Mass Storage",
  .class_id = USB_CLASS_MASS_STORAGE
};

#define mci_lock(x)		lock_acquire(&(x)->lock)
#define mci_unlock(x)		lock_release(&(x)->lock)

struct msc_class_info
{
  int refs;
  struct lock lock;
  struct usb_iface *ui;
  int blk_size;
  int blk_count;
  bool retrying;
  uint32_t tag;
  void *bounce_buffer;

  struct usb_endpoint *eop_in;
  struct usb_endpoint *eop_out;
  struct list_elem peers;
};
static void msc_get_geometry (struct msc_class_info *);
static void msc_io (struct msc_class_info *, block_sector_t, void *buf, bool wr);
static void msc_reset_recovery(struct msc_class_info* mci);
static void msc_bulk_reset(struct msc_class_info* mci);

struct msc_blk_info
{
  struct msc_class_info *mci;
};

static struct list device_list;
static struct block_operations msc_operations;

void usb_storage_init (void);

void
usb_storage_init (void)
{
  list_init (&device_list);
  usb_register_class (&storage_class);
}


static class_info
msc_attached (struct usb_iface *ui)
{
  static int dev_no;

  struct msc_class_info *mci;
  struct msc_blk_info *mbi;
  struct list_elem *li;
  struct block *block;
  char name[16];

  if (ui->subclass_id != USB_SUBCLASS_SCSI)
    {
      printf ("usb_storage: Only support SCSI-type devices\n");
      return NULL;
    }

  mci = malloc (sizeof (struct msc_class_info));

  mci->refs = 0;
  mci->retrying = false;
  lock_init (&mci->lock);
  mci->ui = ui;
  list_push_back (&device_list, &mci->peers);

  /* grab endpoints */
  li = list_begin (&ui->endpoints);
  mci->eop_in = NULL;
  mci->eop_out = NULL;
  while (li != list_end (&ui->endpoints))
    {
      struct usb_endpoint *ue;

      ue = list_entry (li, struct usb_endpoint, peers);
      li = list_next (li);

      if (ue->attr == USB_EOP_ATTR_BULK)
	{
	  if (ue->direction == 0)
	    mci->eop_out = ue;
	  else
	    mci->eop_in = ue;
	}
    }

  ASSERT (mci->eop_in != NULL && mci->eop_out != NULL);

  msc_get_geometry (mci);

  if (mci->blk_size != 512)
    {
      printf ("ignoring device with %d-byte sectors\n", mci->blk_size);
      return NULL;
    }

  mci->bounce_buffer = palloc_get_multiple (PAL_ASSERT,
					    DIV_ROUND_UP (mci->blk_size,
							  PGSIZE));
  mbi = malloc (sizeof (struct msc_blk_info));
  mbi->mci = mci;
  snprintf (name, sizeof name, "ud%c", 'a' + dev_no++);
  block = block_register (name, BLOCK_RAW, "USB", mci->blk_count,
                          &msc_operations, mbi);
  partition_scan (block);
                          
  return mci;
}

static void
msc_detached (class_info ci UNUSED)
{
  PANIC ("msc_detached: STUB");
}

static void
msc_read (void *mbi_, block_sector_t sector, void *buffer) 
{
  struct msc_blk_info *mbi = mbi_;

  mci_lock (mbi->mci);
  msc_io (mbi->mci, sector, buffer, false);
  mci_unlock (mbi->mci);
}

static void
msc_write (void *mbi_, block_sector_t sector, const void *buffer)
{
  struct msc_blk_info *mbi = mbi_;

  mci_lock (mbi->mci);
  msc_io (mbi->mci, sector, (void *) buffer, true);
  mci_unlock (mbi->mci);
}

static struct block_operations msc_operations = 
  {
    msc_read,
    msc_write,
  };

static void
msc_get_geometry (struct msc_class_info *mci)
{
  struct msc_cbw cbw;
  struct scsi_capacity10 *cap;
  uint8_t buf[sizeof (struct msc_csw) + sizeof (struct scsi_capacity10)];
  struct scsi_cdb10 *cdb;
  struct msc_csw *csw;
  int tx;

  /* cap + csw must be read in one shot, combine into a single buffer */
  cap = (struct scsi_capacity10 *) (buf);
  csw = (struct msc_csw *) (&buf[sizeof (struct scsi_capacity10)]);

  cbw.sig = CBW_SIG_MAGIC;
  cbw.tag = mci->tag++;
  cbw.tx_len = sizeof (struct scsi_capacity10);
  cbw.flags = CBW_FL_IN;
  cbw.lun = 0;
  cbw.cb_len = sizeof (struct scsi_cdb10);

  cdb = (void *) (&cbw.cb);
  memset (cdb, 0, sizeof (struct scsi_cdb10));
  cdb->op = SCSI_OP_READ_CAPACITY10;

  usb_dev_bulk (mci->eop_out, &cbw, sizeof (cbw), &tx);
  usb_dev_bulk (mci->eop_in, &buf, sizeof (buf), &tx);

  mci->blk_count = be32_to_machine (cap->blocks) + 1;
  mci->blk_size = be32_to_machine (cap->block_len);

  /* did CSW stall?  */
  if (tx == sizeof (struct scsi_capacity10))
    {
      msc_reset_endpoint (mci->eop_in);
      usb_dev_bulk (mci->eop_in, csw, sizeof (*csw), &tx);
    }

  ASSERT (csw->sig == CSW_SIG_MAGIC);

  if (csw->status != CSW_STATUS_PASSED)
    {
      PANIC ("USB storage geometry read failure!\n");
    }

}

static void
msc_io (struct msc_class_info *mci, block_sector_t bn, void *buf, bool wr)
{
  struct msc_cbw cbw;
  struct msc_csw csw;
  struct scsi_cdb10 *cdb;
  int tx;
  int err;

  if (wr)
    memcpy (mci->bounce_buffer, buf, mci->blk_size);

  memset (&cbw, 0, sizeof (cbw));
  cbw.sig = CBW_SIG_MAGIC;
  cbw.tag = mci->tag++;
  cbw.tx_len = mci->blk_size;
  cbw.flags = (wr) ? CBW_FL_OUT : CBW_FL_IN;
  cbw.lun = 0;
  cbw.cb_len = sizeof (struct scsi_cdb10);

  cdb = (void *) (&cbw.cb);
  cdb->op = (wr) ? SCSI_OP_WRITE10 : SCSI_OP_READ10;
  *((uint32_t *) ((uint8_t *) (&cdb->lba) + 1)) = machine_to_be24 (bn);
  cdb->len = machine_to_be16 (1);

//  msc_reset_endpoint (mci->eop_in);
//  msc_reset_endpoint (mci->eop_out);

  /* set it up */
  err = usb_dev_bulk (mci->eop_out, &cbw, sizeof (cbw), &tx);
  if (err != 0)
    {
      msc_reset_endpoint (mci->eop_out);
      err = usb_dev_bulk (mci->eop_out, &cbw, sizeof (cbw), &tx);
    }

  /* do storage io */
  err = usb_dev_bulk ((wr) ? mci->eop_out : mci->eop_in,
		      mci->bounce_buffer, mci->blk_size, &tx);
  memset (&csw, 0, sizeof (csw));
  ASSERT (tx == mci->blk_size);


  /* get command status */
  err = usb_dev_bulk (mci->eop_in, &csw, sizeof (csw), &tx);
  if (err != 0)
    {
      msc_reset_endpoint (mci->eop_in);
      msc_reset_endpoint (mci->eop_out);
      err = usb_dev_bulk (mci->eop_in, &csw, sizeof (csw), &tx);
      if (err != 0)
	PANIC ("msc_io: error %d\n", err);
    }

  if (csw.sig != CSW_SIG_MAGIC)
    {
      if (mci->retrying == true)
        PANIC ("usb_msd: CSW still missing. Bail out\n");
      printf ("usb_msd: no command status, resetting. Buggy device?\n");
      msc_reset_recovery(mci);
      printf ("reset complete\n");
      mci->retrying = true;
      msc_io (mci, bn, buf, wr);
      return;
    }
  mci->retrying = false;

  if (csw.status != CSW_STATUS_PASSED)
    {
      PANIC ("USB storage IO failure! - error %d\n", csw.status);
    }
  if (!wr)
    memcpy (buf, mci->bounce_buffer, mci->blk_size);
}

static void
msc_reset_endpoint (struct usb_endpoint *eop)
{
  struct usb_setup_pkt sp;

  sp.recipient = USB_SETUP_RECIP_ENDPT;
  sp.type = USB_SETUP_TYPE_STD;
  sp.direction = 0;
  sp.request = REQ_STD_CLR_FEAT;
  sp.value = 0;			/* 0 is ENDPOINT_HALT */
  sp.index = eop->eop;
  sp.length = 0;
  usb_dev_setup (eop, true, &sp, NULL, 0);
}

static void
msc_reset_recovery(struct msc_class_info* mci)
{
  msc_bulk_reset (mci);
  msc_reset_endpoint (mci->eop_in);
  msc_reset_endpoint (mci->eop_out);
}

static void msc_bulk_reset(struct msc_class_info* mci)
{
  struct usb_setup_pkt sp;

  sp.recipient = USB_SETUP_RECIP_DEV;
  sp.type = USB_SETUP_TYPE_CLASS;
  sp.direction = 0;
  sp.request = 0xff;
  sp.value = 0;		 
  sp.index = mci->ui->iface_num;
  sp.length = 0;
  usb_dev_setup (&mci->ui->dev->cfg_eop, true, &sp, NULL, 0);
}
