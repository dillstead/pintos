#include "disk.h"
#include <stdbool.h>
#include "debug.h"
#include "io.h"
#include "interrupt.h"
#include "lib.h"
#include "synch.h"
#include "timer.h"

#define reg_data(CHANNEL) ((CHANNEL)->reg_base + 0)
#define reg_error(CHANNEL) ((CHANNEL)->reg_base + 1)
#define reg_nsect(CHANNEL) ((CHANNEL)->reg_base + 2)
#define reg_lbal(CHANNEL) ((CHANNEL)->reg_base + 3)
#define reg_lbam(CHANNEL) ((CHANNEL)->reg_base + 4)
#define reg_lbah(CHANNEL) ((CHANNEL)->reg_base + 5)
#define reg_device(CHANNEL) ((CHANNEL)->reg_base + 6)
#define reg_status(CHANNEL) ((CHANNEL)->reg_base + 7)
#define reg_command(CHANNEL) reg_status (CHANNEL)
#define reg_ctl(CHANNEL) ((CHANNEL)->reg_base + 0x206)
#define reg_alt_status(CHANNEL) reg_ctl (CHANNEL)

/* Alternate Status Register bits. */
#define STA_BSY 0x80
#define STA_DRDY 0x40
#define STA_DF 0x20
#define STA_DSC 0x10
#define STA_DRQ 0x08
#define STA_CORR 0x04
#define STA_IDX 0x02
#define STA_ERR 0x01

/* Control Register bits. */
#define CTL_SRST 0x04
#define CTL_NIEN 0x02

/* Device Register bits. */
#define DEV_MBS 0xa0            /* Must be set. */
#define DEV_LBA 0x40            /* Linear based addressing. */
#define DEV_DEV 0x10            /* Select device: 0=master, 1=slave. */

/* Commands. */
#define CMD_IDENTIFY 0xec       /* IDENTIFY DEVICE. */

struct disk 
  {
    char name[8];
    struct channel *channel;
    int device;

    bool is_ata;
    disk_sector_no capacity;
  };

struct channel 
  {
    char name[8];
    uint16_t reg_base;
    uint8_t irq;

    bool expecting_interrupt;
    struct semaphore completion_wait;

    struct disk dev[2];
  };

#define CHANNEL_CNT (sizeof channels / sizeof *channels)
static struct channel channels[2];

static void
select_device (const struct disk *d)
{
  struct channel *c = d->channel;
  uint8_t dev = DEV_MBS;
  if (d->device == 1)
    dev |= DEV_DEV;
  outb (reg_device (c), dev);
  inb (reg_alt_status (c));
  timer_nsleep (400);
}

static void
wait_idle (const struct disk *d) 
{
  int i;

  for(i = 0; i < 1000; i++) 
    {
      if ((inb (reg_status (d->channel)) & (STA_BSY | STA_DRQ)) == 0)
        return;
      timer_usleep (10);
    }

  printk ("%s: idle timeout\n", d->name);
}

static void
select_device_wait (const struct disk *d) 
{
  wait_idle (d);
  select_device (d);
  wait_idle (d);
}

/* Wait up to 30 seconds for disk D to clear BSY. */
static bool
busy_wait (const struct disk *d) 
{
  struct channel *c = d->channel;
  int i;
  
  for (i = 0; i < 3000; i++)
    {
      if (i == 700)
        printk ("%s: busy, waiting...", d->name);
      if (!(inb (reg_alt_status (c)) & STA_BSY)) 
        {
          if (i >= 700)
            printk ("ok\n");
          return true; 
        }
      timer_msleep (10);
    }

  printk ("failed\n");
  return false;
}

static void
reset_channel (struct channel *c) 
{
  bool present[2];
  int device;

  /* The ATA reset sequence depends on which devices are present,
     so we start by detecting device presence. */
  for (device = 0; device < 2; device++)
    {
      struct disk *d = &c->dev[device];

      select_device (d);

      outb (reg_nsect (c), 0x55);
      outb (reg_lbal (c), 0xaa);

      outb (reg_nsect (c), 0xaa);
      outb (reg_lbal (c), 0x55);

      outb (reg_nsect (c), 0x55);
      outb (reg_lbal (c), 0xaa);

      present[device] = (inb (reg_nsect (c)) == 0x55
                         && inb (reg_lbal (c)) == 0xaa);
    }

  /* Issue soft reset sequence, which selects device 0 as a side effect.
     Also enable interrupts. */
  outb (reg_ctl (c), 0);
  timer_usleep (10);
  outb (reg_ctl (c), CTL_SRST);
  timer_usleep (10);
  outb (reg_ctl (c), 0);

  timer_msleep (150);

  /* Wait for device 0 to clear BSY. */
  if (present[0]) 
    {
      select_device (&c->dev[0]);
      busy_wait (&c->dev[0]); 
    }

  /* Wait for device 1 to clear BSY. */
  if (present[1])
    {
      int i;

      select_device (&c->dev[1]);
      for (i = 0; i < 3000; i++) 
        {
          if (inb (reg_nsect (c)) == 1 && inb (reg_lbal (c)) == 1)
            break;
          timer_msleep (10);
        }
      busy_wait (&c->dev[1]);
    }
}

static bool
check_ata_device (struct disk *d) 
{
  struct channel *c = d->channel;
  uint8_t error, lbam, lbah;

  select_device (d);

  error = inb (reg_error (c));
  lbam = inb (reg_lbam (c));
  lbah = inb (reg_lbah (c));

  if (error != 1 && (error != 0x81 || d->device == 1)) 
    {
      d->is_ata = false;
      return error != 0x81;      
    }
  else 
    {
      d->is_ata = (lbam == 0 && lbah == 0) || (lbam == 0x3c && lbah == 0xc3);
      return true; 
    }
}

static void
execute_command (struct disk *d, uint8_t command) 
{
  struct channel *c = d->channel;

  /* Interrupts must be enabled or our semaphore will never be
     up'd by the completion handler. */
  ASSERT (intr_get_level () == IF_ON);

  /* Atomically note that we expect an interrupt and send the
     command to the device. */
  intr_disable ();
  c->expecting_interrupt = true;
  outb (reg_command (c), command);
  intr_enable ();

  /* Wait for the command to complete. */
  sema_down (&c->completion_wait);
}

static bool
input_sector (struct channel *c, void *sector) 
{
  uint8_t status;

  ASSERT (sector != NULL);

  status = inb (reg_status (c));
  if (status & STA_DRQ) 
    {
      /* Command was successful.  Read data into SECTOR. */
      insw (reg_data (c), sector, DISK_SECTOR_SIZE / 2);
      return true; 
    }
  else 
    {
      /* Command failed. */
      return false; 
    }
}

static void
identify_ata_device (struct disk *d) 
{
  uint16_t id[DISK_SECTOR_SIZE / 2];

  ASSERT (d->is_ata);

  select_device_wait (d);
  execute_command (d, CMD_IDENTIFY);
  busy_wait (d);

  if (!input_sector (d->channel, id)) 
    {
      d->is_ata = false;
      return;
    }

  d->capacity = id[57] | ((uint32_t) id[58] << 16);
  printk ("%s: detected %"PRDSNu" sector (%d MB) disk\n",
          d->name, d->capacity, d->capacity * DISK_SECTOR_SIZE / 1024 / 1024);
}

static void
interrupt_handler (struct intr_frame *f) 
{
  struct channel *c;

  for (c = channels; c < channels + CHANNEL_CNT; c++)
    if (f->vec_no == c->irq)
      {
        if (c->expecting_interrupt) 
          sema_up (&c->completion_wait);
        else
          printk ("%s: unexpected interrupt\n", c->name);
        return;
      }

  NOT_REACHED ();
}

void
disk_init (void) 
{
  size_t channel;

  for (channel = 0; channel < CHANNEL_CNT; channel++)
    {
      struct channel *c = &channels[channel];
      int device;

      /* Initialize channel. */
      snprintf (c->name, sizeof c->name, "ide%d", channel);
      switch (channel) 
        {
        case 0:
          c->reg_base = 0x1f0;
          c->irq = 14 + 0x20;
          break;
        case 1:
          c->reg_base = 0x170;
          c->irq = 15 + 0x20;
          break;
        default:
          NOT_REACHED ();
        }
      c->expecting_interrupt = false;
      sema_init (&c->completion_wait, 0, c->name);
 
      /* Initialize devices. */
      for (device = 0; device < 2; device++)
        {
          struct disk *d = &c->dev[device];
          snprintf (d->name, sizeof d->name, "%s:%d", c->name, device);
          d->channel = c;
          d->device = device;

          d->is_ata = false;
          d->capacity = 0;
        }

      /* Register interrupt handler. */
      intr_register (c->irq, 0, IF_OFF, interrupt_handler, c->name);

      /* Reset hardware. */
      reset_channel (c);

      /* Detect ATA devices. */
      if (check_ata_device (&c->dev[0]))
        check_ata_device (&c->dev[1]);

      /* Detect device properties. */
      for (device = 0; device < 2; device++)
        if (c->dev[device].is_ata)
          identify_ata_device (&c->dev[device]);
    }
}

struct disk *
disk_get (int idx) 
{
  ASSERT (idx >= 0 && idx < 4);
  return &channels[idx / 2].dev[idx % 2];
}

disk_sector_no disk_size (struct disk *);
void disk_read (struct disk *, disk_sector_no, void *);
void disk_write (struct disk *, disk_sector_no, const void *);
